/*++

Copyright (c) 2022  Shorthorn Project

Module Name:

    synch.c

Abstract:

    This module implements all NTAPI syncronization
    objects.

Author:

    Skulltrail 18-October-2022

Revision History:

--*/

#include <main.h>
#include <config.h>
#include <port.h>

#ifdef _WIN64
#define InterlockedBitTestAndSetPointer(ptr,val) InterlockedBitTestAndSet64((PLONGLONG)ptr,(LONGLONG)val)
#define InterlockedAddPointer(ptr,val) InterlockedAdd64((PLONGLONG)ptr,(LONGLONG)val)
#define InterlockedAndPointer(ptr,val) InterlockedAnd64((PLONGLONG)ptr,(LONGLONG)val)
#define InterlockedOrPointer(ptr,val) InterlockedOr64((PLONGLONG)ptr,(LONGLONG)val)
#else
#define InterlockedBitTestAndSetPointer(ptr,val) InterlockedBitTestAndSet((PLONG)ptr,(LONG)val)
#define InterlockedAddPointer(ptr,val) InterlockedAdd((PLONG)ptr,(LONG)val)
#define InterlockedAndPointer(ptr,val) InterlockedAnd((PLONG)ptr,(LONG)val)
#define InterlockedOrPointer(ptr,val) InterlockedOr((PLONG)ptr,(LONG)val)
#endif

#define InterlockedExchangeAdd16 _InterlockedExchangeAdd16 

#define COND_VAR_UNUSED_FLAG         ((ULONG_PTR)1)
#define COND_VAR_LOCKED_FLAG         ((ULONG_PTR)2)
#define COND_VAR_FLAGS_MASK          ((ULONG_PTR)3)
#define COND_VAR_ADDRESS_MASK        (~COND_VAR_FLAGS_MASK)

#define RtlpWaitOnAddressSpinCount 1024

DWORD ConditionVariableSpinCount=1024;
DWORD SRWLockSpinCount=1024;

typedef SIZE_T SYNCSTATUS;

#define CVF_Full	7	//唤醒申请已满，全部唤醒
#define CVF_Link	8	//修改链表的操作进行中

typedef struct _COND_VAR_WAIT_ENTRY
{
    /* ListEntry must have an alignment of at least 32-bits, since we
       want COND_VAR_ADDRESS_MASK to cover all of the address. */
    LIST_ENTRY ListEntry;
    PVOID WaitKey;
    BOOLEAN ListRemovalHandled;
} COND_VAR_WAIT_ENTRY, * PCOND_VAR_WAIT_ENTRY;

typedef struct _ADDRESS_WAIT_BLOCK
{
	volatile void* Address;
	//因为Windows 8以及更高版本才支持 ZwWaitForAlertByThreadId，所以我们直接把 ThreadId 砍掉了，反正没鸟用
	//ULONG_PTR            ThreadId;

	// 它是后继
	struct _ADDRESS_WAIT_BLOCK* back;
	// 它是前驱
	struct _ADDRESS_WAIT_BLOCK* notify;
	// 似乎指向Root，但是Root时才指向自己，其余情况为 nullptr，这是一种安全性？
	struct _ADDRESS_WAIT_BLOCK* next;
	volatile long         flag;
} ADDRESS_WAIT_BLOCK;

#define CONTAINING_COND_VAR_WAIT_ENTRY(address, field) \
    CONTAINING_RECORD(address, COND_VAR_WAIT_ENTRY, field)
	
#define ADDRESS_GET_BLOCK(AW) ((ADDRESS_WAIT_BLOCK*)((SIZE_T)(AW) & (~(SIZE_T)(0x3))))

BOOL NTAPI RtlpWaitCouldDeadlock();

BOOL NTAPI RtlDllShutdownInProgress(VOID);

static 
NTSTATUS 
RtlpWaitOnAddressWithTimeout(
	ADDRESS_WAIT_BLOCK* pWaitBlock, 
	LARGE_INTEGER *TimeOut
);

static 
void 
RtlpWaitOnAddressRemoveWaitBlock(
	ADDRESS_WAIT_BLOCK* pWaitBlock
);

static void RtlpWaitOnAddressRemoveWaitBlock(ADDRESS_WAIT_BLOCK* pWaitBlock);

static inline int interlocked_dec_if_nonzero( volatile long int *dest )
{
     int val, tmp;
     for (val = *dest;; val = tmp)
     {
         if (!val || (tmp = InterlockedCompareExchange( dest, val - 1, val )) == val)
             break;
     }
     return val;
}

/* INTERNAL FUNCTIONS ********************************************************/

FORCEINLINE
ULONG_PTR
InternalCmpXChgCondVarAcq(IN OUT PRTL_CONDITION_VARIABLE ConditionVariable,
                          IN ULONG_PTR Exchange,
                          IN ULONG_PTR Comperand)
{
    return (ULONG_PTR)InterlockedCompareExchangePointerAcquire(&ConditionVariable->Ptr,
                                                               (PVOID)Exchange,
                                                               (PVOID)Comperand);
}

FORCEINLINE
ULONG_PTR
InternalCmpXChgCondVarRel(IN OUT PRTL_CONDITION_VARIABLE ConditionVariable,
                          IN ULONG_PTR Exchange,
                          IN ULONG_PTR Comperand)
{
    return (ULONG_PTR)InterlockedCompareExchangePointerRelease(&ConditionVariable->Ptr,
                                                               (PVOID)Exchange,
                                                               (PVOID)Comperand);
}

/* GLOBALS *******************************************************************/

extern HANDLE GlobalKeyedEventHandle;
static HANDLE WaitOnAddressKeyedEventHandle;
static RTL_RUN_ONCE init_once_woa = RTL_RUN_ONCE_INIT; 

VOID
RtlpInitializeKeyedEvent(VOID)
{
    ASSERT(GlobalKeyedEventHandle == NULL);
    NtCreateKeyedEvent(&GlobalKeyedEventHandle, EVENT_ALL_ACCESS, NULL, 0);
}

VOID
RtlpCloseKeyedEvent(VOID)
{
    ASSERT(GlobalKeyedEventHandle != NULL);
    NtClose(GlobalKeyedEventHandle);
    GlobalKeyedEventHandle = NULL;
}

static DWORD NTAPI
RtlpInitializeWaitOnAddressKeyedEvent( RTL_RUN_ONCE *once, void *param, void **context )
{
    NtCreateKeyedEvent(&WaitOnAddressKeyedEventHandle, GENERIC_READ|GENERIC_WRITE, NULL, 0);
	return TRUE; 
}


static ULONG_PTR* GetBlockByWaitOnAddressHashTable(LPVOID Address)
{
	static volatile ULONG_PTR WaitOnAddressHashTable[128];

	size_t Index = ((size_t)Address >> 5) & 0x7F;

	return &WaitOnAddressHashTable[Index];
}

VOID InitializeGlobalKeyedEventHandle()
{
	HANDLE KeyedEventHandle;
	//Windows XP等平台则 使用系统自身的 CritSecOutOfMemoryEvent，Vista或者更高平台 我们直接返回 nullptr 即可。
	if (GlobalKeyedEventHandle == NULL)
	{
		const wchar_t Name[] = L"\\KernelObjects\\CritSecOutOfMemoryEvent";

		UNICODE_STRING ObjectName = {sizeof(Name) - sizeof(wchar_t),sizeof(Name) - sizeof(wchar_t) ,(PWSTR)Name };
		OBJECT_ATTRIBUTES attr = { sizeof(attr),0,&ObjectName };

		if (NtOpenKeyedEvent(&KeyedEventHandle, MAXIMUM_ALLOWED, &attr) < 0)
		{
			RtlRaiseStatus(STATUS_RESOURCE_NOT_OWNED);
		}

		if (InterlockedCompareExchange((volatile long *)&GlobalKeyedEventHandle, (size_t)KeyedEventHandle, (size_t)0))
		{
			RtlFreeHeap( RtlGetProcessHeap(), 0, KeyedEventHandle );
		}
	}
}

static void RtlpWaitOnAddressWakeEntireList(ADDRESS_WAIT_BLOCK* pBlock)
{
	for (; pBlock;)
	{
		ADDRESS_WAIT_BLOCK* Tmp = pBlock->back;

		if (InterlockedExchange((volatile long *)&pBlock->flag, 2) == 0)
		{
			NtReleaseKeyedEvent(GlobalKeyedEventHandle, pBlock, 0, 0);
		}

		pBlock = Tmp;
	}
}

static void RtlpOptimizeWaitOnAddressWaitList(volatile ULONG_PTR* ppFirstBlock)
{
	ULONG_PTR Current = *ppFirstBlock;
	ADDRESS_WAIT_BLOCK* pBlock;
	ADDRESS_WAIT_BLOCK* pItem;
	ADDRESS_WAIT_BLOCK* Tmp;
	size_t Last;

	for (;;)
	{
		pBlock = ADDRESS_GET_BLOCK(Current);

		for (pItem = pBlock;;)
		{
			if (pItem->next != 0)
			{
				pBlock->next = pItem->next;
				break;
			}

			Tmp = pItem;
			pItem = pItem->back;
			pItem->notify = Tmp;
		}

		Last = InterlockedCompareExchange((volatile long *)ppFirstBlock, (Current & 1) == 0 ? (size_t)(pBlock) : 0, Current);

		if (Last == Current)
		{
			if(Current & 1)
			{
				RtlpWaitOnAddressWakeEntireList(pBlock);
			}

			return;
		}


		Current = Last;
	}
}

static void RtlpAddWaitBlockToWaitList(ADDRESS_WAIT_BLOCK* pWaitBlock)
{
	ULONG_PTR* ppFirstBlock = GetBlockByWaitOnAddressHashTable((LPVOID)pWaitBlock->Address);
	ULONG_PTR Current = *ppFirstBlock;	
	size_t New;
	ADDRESS_WAIT_BLOCK* back;
	size_t Last;

	for (;;)
	{
		New = (size_t)(pWaitBlock) | ((size_t)(Current) & 0x3);

		back = ADDRESS_GET_BLOCK(Current);
	    pWaitBlock->back = back;
		if (back)
		{
			New |= 0x2;

			pWaitBlock->next = 0;
		}
		else
		{
			pWaitBlock->next = pWaitBlock;
		}

		Last = InterlockedCompareExchange((volatile long *)ppFirstBlock, New, Current);

		if (Last == Current)
		{
			//0x2状态发生变化 才需要重新优化锁。
			if ((Current ^ New) & 0x2)
			{
				RtlpOptimizeWaitOnAddressWaitList(ppFirstBlock);
			}

			return;
		}

		Current = Last;
	}
}

/***********************************************************************
 *           RtlWaitOnAddress   (NTDLL.@)
 */

static 
NTSTATUS 
RtlpWaitOnAddressWithTimeout(
	ADDRESS_WAIT_BLOCK* pWaitBlock, 
	LARGE_INTEGER *TimeOut
)
{
	NTSTATUS Status;
	ULONG SpinCount;
	//单核 我们无需自旋，直接进入等待过程即可
	if (NtCurrentTeb()->ProcessEnvironmentBlock->NumberOfProcessors > 1 && RtlpWaitOnAddressSpinCount)
	{
		for (SpinCount = 0; SpinCount < RtlpWaitOnAddressSpinCount;++SpinCount)
		{
			if ((pWaitBlock->flag & 1) == 0)
			{
				//自旋过程中，等到了信号改变
				return STATUS_SUCCESS;
			}

			YieldProcessor();
		}
	}

	if (!_interlockedbittestandreset((volatile long *)&pWaitBlock->flag, 0))
	{
		//本来我是拒绝的，但是运气好，状态已经发生了反转
		return STATUS_SUCCESS;
	}

	Status = NtWaitForKeyedEvent(GlobalKeyedEventHandle, pWaitBlock, 0, TimeOut);

	if (Status == STATUS_TIMEOUT)
	{
		if (InterlockedExchange((volatile long *)&pWaitBlock->flag, 4) == 2)
		{
			Status = NtWaitForKeyedEvent(GlobalKeyedEventHandle, pWaitBlock, 0, 0);
		}
		else
		{
			RtlpWaitOnAddressRemoveWaitBlock(pWaitBlock);
		}
	}

	return Status;
}	
	
static void RtlpWaitOnAddressRemoveWaitBlock(ADDRESS_WAIT_BLOCK* pWaitBlock)
{
	ULONG_PTR* ppFirstBlock = GetBlockByWaitOnAddressHashTable((LPVOID)pWaitBlock->Address);
	ULONG_PTR Current = *ppFirstBlock;
	size_t Last;
	size_t New;
	ADDRESS_WAIT_BLOCK* pBlock;
	ADDRESS_WAIT_BLOCK* pItem;
	ADDRESS_WAIT_BLOCK* pNotify;
	ADDRESS_WAIT_BLOCK* Tmp;
	BOOL bFind;

	for (; Current; Current = Last)
	{
		if (Current & 2)
		{
			Last = InterlockedCompareExchange((volatile long *)ppFirstBlock, Current | 1, Current);

			if (Last == Current)
			{
				break;
			}
		}else{
			New = Current | 0x2;
			Last = InterlockedCompareExchange((volatile long *)ppFirstBlock, New, Current);

			if (Last == Current)
			{
				Current = New;

				bFind = FALSE;

							//同步成功！
				pBlock = ADDRESS_GET_BLOCK(New);
				pItem = pBlock;

				pNotify = pBlock->notify;				

				do
				{
					Tmp = pBlock->back;

					if (pBlock != pWaitBlock)
					{
						pBlock->notify = pNotify;
						pNotify = pBlock;


						pBlock = Tmp;
						Tmp = pItem;
						continue;
					}

					bFind = TRUE;


					if (pBlock != pItem)
					{
						pNotify->back = Tmp;
						if (Tmp)
							Tmp->notify = pNotify;
						else
							pNotify->next = pNotify;

						pBlock = Tmp;
						Tmp = pItem;
						continue;
					}

					New = (size_t)(pBlock->back);
					if (Tmp)
					{
						New = (size_t)(Tmp) ^ (Current ^ (size_t)(Tmp)) & 0x3;
					}

					Last = InterlockedCompareExchange((volatile long *)ppFirstBlock, New, Current);

					if (Last == Current)
					{
						if (New == 0)
							return;

						Tmp->notify = 0;
						pBlock = Tmp;
					}else{
						Current = Last;

						Tmp = pBlock = ADDRESS_GET_BLOCK(Current);
						pNotify = pBlock->notify;
					}


					pItem = Tmp;
				} while (pBlock);
							

				if (bFind == FALSE && InterlockedExchange((volatile long *)&pWaitBlock->flag, 0) != 2)
				{
					NtWaitForKeyedEvent(GlobalKeyedEventHandle, pWaitBlock, 0, 0);
				}

				Tmp->next = pNotify;

				for (;;)
				{
					Last = InterlockedCompareExchange((volatile long *)ppFirstBlock, (Current & 1) == 0 ? (size_t)(ADDRESS_GET_BLOCK(Current)) : 0, Current);

					if (Last == Current)
						break;

					Current = Last;
				}

				if (Current & 1)
					RtlpWaitOnAddressWakeEntireList(ADDRESS_GET_BLOCK(Current));


					return;
			}
		}
	}

	if (InterlockedExchange((volatile long *)&pWaitBlock->flag, 1) == 2)
		return;

	RtlpWaitOnAddressWithTimeout(pWaitBlock, 0);
}  
								  
NTSTATUS 
NTAPI
RtlWaitOnAddress( 
	const void *Address, 
	const void *CompareAddress, 
	SIZE_T AddressSize,
    const LARGE_INTEGER *TimeOut )	
{		
	BOOL bSame;
	NTSTATUS Status;
	ADDRESS_WAIT_BLOCK WaitBlock;
			
	if (AddressSize > 8 || AddressSize == 0 || ((AddressSize - 1) & AddressSize) != 0)
	{
		return STATUS_INVALID_PARAMETER;
	}
	
	WaitBlock.Address = Address;
	WaitBlock.back = 0;
	WaitBlock.notify = 0;
	WaitBlock.next = 0;
	WaitBlock.flag = 1;

	RtlpAddWaitBlockToWaitList(&WaitBlock);
			
	switch (AddressSize)
	{
		case 1:
			bSame = *(volatile byte*)Address == *(volatile byte*)CompareAddress;
			break;
		case 2:
			bSame = *(volatile WORD*)Address == *(volatile WORD*)CompareAddress;
			break;
		case 4:
			bSame = *(volatile DWORD*)Address == *(volatile DWORD*)CompareAddress;
			break;
		default:
			//case 8:
#if _WIN64
			//64位自身能保证操作的原子性
			bSame = *(volatile unsigned long long*)Address == *(volatile unsigned long long*)CompareAddress;
#else
			bSame = InterlockedCompareExchange64((volatile long long*)Address, 0, 0) == *(volatile long long*)CompareAddress;
#endif
			break;
	}

	if (!bSame)
	{
		//结果不相同，我们从等待队列移除
		RtlpWaitOnAddressRemoveWaitBlock(&WaitBlock);
		return TRUE;
	}			

	Status = RtlpWaitOnAddressWithTimeout(&WaitBlock, TimeOut);

	return Status;	
}

/***********************************************************************
 *           RtlWakeAddressAll    (NTDLL.@)
 */
void NTAPI RtlWakeAddressAll( const void *addr )
{
    LARGE_INTEGER now;

    RtlRunOnceExecuteOnce( &init_once_woa, RtlpInitializeWaitOnAddressKeyedEvent, NULL, NULL );
    NtQuerySystemTime( &now );
    while (NtReleaseKeyedEvent( GlobalKeyedEventHandle, addr, 0, &now ) == STATUS_SUCCESS) {}
}

/***********************************************************************
 *           RtlWakeAddressSingle (NTDLL.@)
 */
void NTAPI RtlWakeAddressSingle( const void *addr )
{
    LARGE_INTEGER now;

    RtlRunOnceExecuteOnce( &init_once_woa, RtlpInitializeWaitOnAddressKeyedEvent, NULL, NULL );
    NtQuerySystemTime( &now );
    NtReleaseKeyedEvent( GlobalKeyedEventHandle, addr, 0, &now );
}  

/******************************************************************
 *              RtlRunOnceBeginInitialize (NTDLL.@)
 */
DWORD 
NTAPI 
RtlRunOnceBeginInitialize( 
	RTL_RUN_ONCE *once, 
	ULONG flags, 
	void **context 
)
{
    if (flags & RTL_RUN_ONCE_CHECK_ONLY)
    {
        ULONG_PTR val = (ULONG_PTR)once->Ptr;

        if (flags & RTL_RUN_ONCE_ASYNC) return STATUS_INVALID_PARAMETER;
        if ((val & 3) != 2) return STATUS_UNSUCCESSFUL;
        if (context) *context = (void *)(val & ~3);
        return STATUS_SUCCESS;
    } 

    for (;;)
    {
        ULONG_PTR next, val = (ULONG_PTR)once->Ptr;

        switch (val & 3)
        {
        case 0:  /* first time */
            if (!interlocked_cmpxchg_ptr( &once->Ptr,
                                          (flags & RTL_RUN_ONCE_ASYNC) ? (void *)3 : (void *)1, 0 ))
                return STATUS_PENDING;
            break;

        case 1:  /* in progress, wait */			
            if (flags & RTL_RUN_ONCE_ASYNC) return STATUS_INVALID_PARAMETER;
            next = val & ~3;
            if (interlocked_cmpxchg_ptr( &once->Ptr, (void *)((ULONG_PTR)&next | 1),
                                         (void *)val ) == (void *)val)
                NtWaitForKeyedEvent( GlobalKeyedEventHandle, &next, FALSE, NULL );
            break;

        case 2:  /* done */
            if (context) *context = (void *)(val & ~3);
            return STATUS_SUCCESS;

        case 3:  /* in progress, async */
            if (!(flags & RTL_RUN_ONCE_ASYNC)) return STATUS_INVALID_PARAMETER;
            return STATUS_PENDING;
        }
    }
}

/*
 * @implemented - need test
 */
/******************************************************************
 *              RtlRunOnceComplete (NTDLL.@)
 */
DWORD
NTAPI
RtlRunOnceComplete( 
	PRTL_RUN_ONCE once, 
	ULONG flags, 
	PVOID context 
)
{
    if ((ULONG_PTR)context & 3) return STATUS_INVALID_PARAMETER;

    if (flags & RTL_RUN_ONCE_INIT_FAILED)
    {
        if (context) return STATUS_INVALID_PARAMETER;
        if (flags & RTL_RUN_ONCE_ASYNC) return STATUS_INVALID_PARAMETER;
    }
    else context = (void *)((ULONG_PTR)context | 2);
	
    for (;;)
    {
        ULONG_PTR val = (ULONG_PTR)once->Ptr;

        switch (val & 3)
        {
        case 1:  /* in progress */
            if (interlocked_cmpxchg_ptr( &once->Ptr, context, (void *)val ) != (void *)val) break;
            val &= ~3;
            while (val)
            {
                ULONG_PTR next = *(ULONG_PTR *)val;
                NtReleaseKeyedEvent( GlobalKeyedEventHandle, (void *)val, FALSE, NULL );
                val = next;
            }
            return STATUS_SUCCESS;

        case 3:  /* in progress, async */
            if (!(flags & RTL_RUN_ONCE_ASYNC)) return STATUS_INVALID_PARAMETER;
            if (interlocked_cmpxchg_ptr( &once->Ptr, context, (void *)val ) != (void *)val) break;
            return STATUS_SUCCESS;

        default:
            return STATUS_UNSUCCESSFUL;
        }
    }
}

/******************************************************************
  *              RtlRunOnceExecuteOnce (NTDLL.@)
  */
DWORD 
NTAPI 
RtlRunOnceExecuteOnce( 
	RTL_RUN_ONCE *once, 
	PRTL_RUN_ONCE_INIT_FN func,
    void *param, void **context 
)
{
     DWORD ret = RtlRunOnceBeginInitialize( once, 0, context );
 
     if (ret != STATUS_PENDING) return ret;
 
     if (!func( once, param, context ))
     {
         RtlRunOnceComplete( once, RTL_RUN_ONCE_INIT_FAILED, NULL );
         return STATUS_UNSUCCESSFUL;
     } 
     return RtlRunOnceComplete( once, 0, context ? *context : NULL );
}

/******************************************************************
 *              RtlRunOnceInitialize (NTDLL.@)
 */
void 
NTAPI 
RtlRunOnceInitialize( 
	RTL_RUN_ONCE *once 
)
{
    once->Ptr = NULL;
}

//New SRW implementation
void NTAPI RtlpInitSRWLock(PEB* pPEB)
{
	if (pPEB->NumberOfProcessors==1)
		SRWLockSpinCount=0;
}

void NTAPI RtlInitializeSRWLock(RTL_SRWLOCK* SRWLock)
{
	SRWLock->Ptr=NULL;
}

void NTAPI RtlpWakeSRWLock(RTL_SRWLOCK* SRWLock,SYNCSTATUS OldStatus)
{
	SYNCSTATUS CurrStatus;
	SYNCITEM* last;
	SYNCITEM* first;	
	
	while (1)
	{
		//已经有线程抢先获取了锁，取消唤醒操作
		if (OldStatus&SRWF_Hold)	//编译器将while(...)编译成if (...) do {} while(...)
		{
			do 
			{
				CurrStatus=InterlockedCompareExchange((volatile long*)SRWLock,OldStatus-SRWF_Link,OldStatus);	//清除链表操作标记
				//状态被其它线程更新，设置状态失败
				//本次分析失效，更新状态重新分析
				//下面有大量类似代码，不再重复说明
				if (CurrStatus==OldStatus)
					return ;
				OldStatus=CurrStatus;
			} while (OldStatus&SRWF_Hold);
		}

		last=(SYNCITEM*)(OldStatus&SRWM_ITEM);
		first=last->first;
		if (first==NULL)
		{
			SYNCITEM* curr=last;
			do 
			{
				curr->back->next=curr;	//补全链表
				curr=curr->back;		//遍历链表
				first=curr->first;		//更新查找结果
			} while (first==NULL);		//找一个有效的first
			//first指针提前到最近的地方
			//优化链表里没有这个判断，大概是插入多个节点需要优化时，first一定不为last
			if (last!=curr)	
				last->first=first;
		}

		//如果后续还有节点等待，且这个是独占请求
		if ((first->next!=NULL) && (first->attr&SYNC_Exclusive))
		{
			last->first=first->next;	//从链表中删除这个节点（删除和优化每次都用最近的first指针）
			first->next=NULL;			//first从原链表脱离
			_InterlockedAnd((long*)SRWLock,(~SRWF_Link));	//链表操作全部完成，去掉标记
			break;
		}
		//否则，可能只有这一个节点等待，或这个是共享请求，全部唤醒
		else
		{
			CurrStatus=InterlockedCompareExchange((volatile long*)SRWLock,0,OldStatus);	//将状态重置为空闲
			if (OldStatus==CurrStatus)
				break;
			last->first=first;	//将找到的first放到最近的位置
			OldStatus=CurrStatus;
		}
	}

	//依次唤醒线程，可能仅有first一个，也可能是first链上的全部
	//如果是全部唤醒，接下来线程会再次争夺锁，抢不到的再次循环，构建链表并阻塞
	//好处是省掉了各种情况的分析，后面几个共享锁将成功获得锁，直到遇到独占锁
	do 
	{
		//抢到锁的线程会返回，栈上的item失效，必须先保存next
		SYNCITEM* next=first->next;
		//如果有SYNC_Spinning标记，表示还在自旋等待，即将进入休眠
		//下面的lock btr将其置0，目标线程发现后跳过休眠
		//如果没有SYNC_Spinning标记，说明目标线程清掉了此标记，正式进入休眠
		//下面的lock btr没有影响，本线程负责将目标线程唤醒
		//需要注意的是，NtReleaseKeyedEvent发现key并没有休眠时，会阻塞当前线程
		//直到有线程用此key调用了NtWaitForKeyedEvent，才会唤醒，因此不会丢失通知
		if (InterlockedBitTestAndReset((LONG*)&(first->attr),SYNC_SPIN_BIT)==0)
			NtReleaseKeyedEvent(GlobalKeyedEventHandle,first,FALSE,NULL);
		first=next;	//遍历链表
	} while (first!=NULL);
}

void NTAPI RtlpOptimizeSRWLockList(RTL_SRWLOCK* SRWLock,SYNCSTATUS OldStatus)
{
	SYNCSTATUS CurrStatus;
	if (OldStatus&SRWF_Hold)
	{
		do 
		{
			SYNCITEM* last=(SYNCITEM*)(OldStatus&SRWM_ITEM);
			if (last!=NULL)
			{
				SYNCITEM* curr=last;
				while (curr->first==NULL)
				{
					curr->back->next=curr;	//补全链表
					curr=curr->back;		//遍历链表
				}
				last->first=curr->first;	//将first放到离容器入口最近的位置，加速下次查找
			}
			//链表操作结束，清除标记
			CurrStatus=InterlockedCompareExchange((volatile long*)SRWLock,OldStatus-SRWF_Link,OldStatus);
			if (CurrStatus==OldStatus)
				return ;
			OldStatus=CurrStatus;
		} while (OldStatus&SRWF_Hold);
	}
	//有人释放了锁，停止优化，改为唤醒
	RtlpWakeSRWLock(SRWLock,OldStatus);
}

void NTAPI RtlAcquireSRWLockExclusive(RTL_SRWLOCK* SRWLock)
{
	//volatile
	__declspec(align(16)) SYNCITEM item;
	BOOL IsOptimize;
	SYNCSTATUS NewStatus;
	DWORD dwBackOffCount=0;
	SYNCSTATUS CurrStatus;
	SYNCSTATUS OldStatus;
	int i;

	//如果当前状态为空闲，直接获取锁
	//甚至某个线程刚释放锁，仅清除了Hold标记，其它线程还没来得及获取锁
	//本线程也可以趁机获取锁，设置标记，令唤醒操作取消或唤醒后再次进入等待
	if (InterlockedBitTestAndSet((LONG*)SRWLock,SRW_HOLD_BIT)==0)
		return ;

	OldStatus=(SYNCSTATUS)(SRWLock->Ptr);
	
	while (1)
	{
		//如果当前已有线程持有锁，本线程将构建节点，将自己加入链表
		if (OldStatus&SRWF_Hold)
		{
			if (RtlpWaitCouldDeadlock())
			{
				//GetCurrentProcess(),STATUS_THREAD_IS_TERMINATING
				NtTerminateProcess((HANDLE)0xFFFFFFFF,0xC000004B);
			}

			item.attr=SYNC_Exclusive|SYNC_Spinning;
			item.next=NULL;
			IsOptimize=FALSE;

			//如果有线程已经在前面等待了，就把之前的节点设为back
			if (OldStatus&SRWF_Wait)
			{
				item.first=NULL;
				item.count=0;
				item.back=(SYNCITEM*)(OldStatus&SRWM_ITEM);
				NewStatus=((SYNCSTATUS)&item)|(OldStatus&SRWF_Many)|(SRWF_Link|SRWF_Wait|SRWF_Hold);

				if (!(OldStatus&SRWF_Link))	//当前没人操作链表，就优化链表
					IsOptimize=TRUE;
			}
			//如果本线程是第一个等待的线程，first指向自己
			//查找时以first指针为准，不需要设置back
			else
			{
				item.first=&item;
				//如果锁的拥有者以独占方式持有，共享计数为0
				//如果锁的拥有者以共享方式持有，共享计数为1或更多
				item.count=OldStatus>>SRW_COUNT_BIT;
				if (item.count>1)
					NewStatus=((SYNCSTATUS)&item)|(SRWF_Many|SRWF_Wait|SRWF_Hold);
				else
					NewStatus=((SYNCSTATUS)&item)|(SRWF_Wait|SRWF_Hold);
			}
			//提交新状态
			CurrStatus=InterlockedCompareExchange((volatile long*)SRWLock,NewStatus,OldStatus);
			if (CurrStatus==OldStatus)
			{
				if (IsOptimize)
					RtlpOptimizeSRWLockList(SRWLock,NewStatus);
				//进入内核的代价太高，先进行一段自旋等待
				for (i=SRWLockSpinCount;i>0;i--)
				{
					if (!(item.attr&SYNC_Spinning))	//其它线程可能唤醒本线程，清除标记
						break;
					_mm_pause();
				}
				//如果一直没能等到唤醒，就进入内核休眠
				if (InterlockedBitTestAndReset((LONG*)(&item.attr),SYNC_SPIN_BIT))
					NtWaitForKeyedEvent(GlobalKeyedEventHandle,&item,FALSE,NULL);
				//被唤醒后再次循环检测条件
				OldStatus=CurrStatus;
			}
			else
			{
				//线程处于激烈的竞争中，退避一段时间
				RtlBackoff(&dwBackOffCount);
				OldStatus=(SYNCSTATUS)(SRWLock->Ptr);
			}
		}
		//别的线程可能做了什么，反正现在没有线程持有锁了，尝试获取锁
		else
		{
			CurrStatus=InterlockedCompareExchange((volatile long*)SRWLock,OldStatus+SRWF_Hold,OldStatus);
			if (CurrStatus==OldStatus)
				return ;
			RtlBackoff(&dwBackOffCount);
			OldStatus=(SYNCSTATUS)(SRWLock->Ptr);
		}
	}
}

void NTAPI RtlAcquireSRWLockShared(RTL_SRWLOCK* SRWLock)
{
	//volatile
	__declspec(align(16)) SYNCITEM item;
	BOOL IsOptimize;
	DWORD dwBackOffCount=0;
	int i;

	SYNCSTATUS NewStatus;
	SYNCSTATUS CurrStatus;
	SYNCSTATUS OldStatus=InterlockedCompareExchange((volatile long*)SRWLock,(1<<SRW_COUNT_BIT)|SRWF_Hold,0);
	//如果当前状态为空闲，直接获取锁
	if (OldStatus==0)
		return ;

	while (1)
	{
		//因独占锁需要等待的情况
		//出于公平性考虑，只要有独占锁请求，后续的所有共享锁请求都要排队（即使当前正处于共享状态）
		//有了wait标记，说明：1.当前是独占锁，后续无论什么类型的请求都要排队
		//2.当前是共享锁，但是队列里有独占锁请求，后来的共享锁也应该排队
		//作为对比，若当前是共享锁，紧接着的共享请求可以直接获取锁，不会阻塞和添加wait标记
		//另有一种特殊情况，当前是独占锁，后续没有线程请求锁，也就没有wait标记
		//但是这种情况的share count为0（作为对比，只有单个共享锁时share count为1）
		//一旦后续有请求，请求者就会等待，变成有wait标记的情况
		if ((OldStatus&SRWF_Hold) && ((OldStatus&SRWF_Wait) || ((OldStatus&SRWM_ITEM)==(SYNCSTATUS)NULL)))
		{
			if (RtlpWaitCouldDeadlock())
				NtTerminateProcess((HANDLE)0xFFFFFFFF,0xC000004B);

			item.attr=SYNC_Spinning;
			item.count=0;
			IsOptimize=FALSE;
			item.next=NULL;

			if (OldStatus&SRWF_Wait)
			{
				item.back=(SYNCITEM*)(OldStatus&SRWM_ITEM);
				//原汇编就是这么写的，但是刚才SRWF_Hold已经检测到了
				NewStatus=((SYNCSTATUS)&item)|(OldStatus&(SRWF_Many|SRWF_Hold))|(SRWF_Link|SRWF_Wait);
				item.first=NULL;

				if (!(OldStatus&SRWF_Link))
					IsOptimize=TRUE;
			}
			else
			{
				item.first=&item;
				//当前一定是独占锁，所以不用考虑SRWF_Many
				NewStatus=((SYNCSTATUS)&item)|(SRWF_Wait|SRWF_Hold);
			}

			CurrStatus=InterlockedCompareExchange((volatile long*)SRWLock,NewStatus,OldStatus);
			if (CurrStatus==OldStatus)
			{
				if (IsOptimize)
					RtlpOptimizeSRWLockList(SRWLock,NewStatus);

				for (i=SRWLockSpinCount;i>0;i--)
				{
					if (!(item.attr&SYNC_Spinning))
						break;
					_mm_pause();
				}

				if (InterlockedBitTestAndReset((LONG*)&(item.attr),SYNC_SPIN_BIT))
					NtWaitForKeyedEvent(GlobalKeyedEventHandle,&item,FALSE,NULL);
				OldStatus=CurrStatus;
			}
			else
			{
				RtlBackoff(&dwBackOffCount);
				OldStatus=(SYNCSTATUS)SRWLock->Ptr;
			}
		}
		else
		{
			//某个线程刚释放锁，仅清除了Hold标记，其它线程还没来得及获取锁
			//本线程可以趁机获取锁，设置标记，令唤醒操作取消或唤醒后再次抢占锁
			//这里有点小问题，如果刚刚是独占锁释放，即使后续是共享请求
			//也有可能取消唤醒操作，而不是和当前的共享线程一起获取锁
			if (OldStatus&SRWF_Wait)
				NewStatus=OldStatus+SRWF_Hold;
			//当前处于共享状态，可以获取锁，增加共享计数
			else
				NewStatus=(OldStatus+(1<<SRW_COUNT_BIT))|SRWF_Hold;
			CurrStatus=InterlockedCompareExchange((volatile long*)SRWLock,NewStatus,OldStatus);
			if (CurrStatus==OldStatus)
				return ;
			RtlBackoff(&dwBackOffCount);
			OldStatus=(SYNCSTATUS)SRWLock->Ptr;
		}
	}
}

void NTAPI RtlReleaseSRWLockExclusive(RTL_SRWLOCK* SRWLock)
{
	//去掉Hold标记
	SYNCSTATUS CurrStatus;
	SYNCSTATUS OldStatus=InterlockedExchangeAdd((volatile long*)SRWLock,-SRWF_Hold);
	// if (!(OldStatus&SRWF_Hold))
		// RtlRaiseStatus(0xC0000264);	//STATUS_RESOURCE_NOT_OWNED
	//有线程在等待，且没有线程正在操作链表，执行唤醒操作
	//否则当前操作链表的线程检测到状态改变，执行唤醒操作
	if ((OldStatus&SRWF_Wait) && !(OldStatus&SRWF_Link))
	{
		OldStatus-=SRWF_Hold;
		CurrStatus=InterlockedCompareExchange((volatile long*)SRWLock,OldStatus+SRWF_Link,OldStatus);
		if (OldStatus==CurrStatus)
			RtlpWakeSRWLock(SRWLock,OldStatus+SRWF_Link);
	}
}

void NTAPI RtlReleaseSRWLockShared(RTL_SRWLOCK* SRWLock)
{
	SYNCSTATUS CurrStatus,NewStatus;
	DWORD count;
	SYNCSTATUS OldStatus=InterlockedCompareExchange((volatile long*)SRWLock,0,((1<<SRW_COUNT_BIT)|SRWF_Hold));
	//如果共享计数为1，且标记仅为Hold
	//说明仅有一个共享锁，恢复至空闲状态就可以了
	if (OldStatus==0x11)//((1<<SRW_COUNT_BIT)|SRWF_Hold))
		return ;

	// if ((OldStatus&SRWF_Hold) == 0)
		// RtlRaiseStatus(0xC0000264);

	//只存在共享锁
	if (!(OldStatus&SRWF_Wait))
	{
		do 
		{
			//共享计数为1，清空为空闲状态
			if ((OldStatus&SRWM_COUNT)<=(1<<SRW_COUNT_BIT))
				NewStatus=0;
			//共享计数大于0，将其-1
			else
				NewStatus=OldStatus-(1<<SRW_COUNT_BIT);
			CurrStatus=InterlockedCompareExchange((volatile long*)SRWLock,NewStatus,OldStatus);
			if (CurrStatus==OldStatus)
				return ;
			OldStatus=CurrStatus;
		} while (!(OldStatus&SRWF_Wait));
	}

	//有独占请求等待时
	//如果有多个共享锁，计数-1
	if (OldStatus&SRWF_Many)
	{
		SYNCITEM* curr=(SYNCITEM*)(OldStatus&SRWM_ITEM);
		//寻找最近的first节点，查询共享计数
		//共享锁接共享锁不会阻塞，也不会新增等待节点
		//共享锁接独占锁，独占锁会等待，并且其item记录共享计数
		//特殊的，独占锁接独占锁，或独占锁接共享锁，记录的共享计数为0
		while (curr->first==NULL)
			curr=curr->back;	
		curr=curr->first;

		//共享计数-1，如果共享计数大于0，说明现在仍有线程占有共享锁
		count=InterlockedDecrement(&curr->count);
		if (count>0)
			return ;
	}

	//共享锁完全释放，唤醒下个等待者
	while (1)
	{
		NewStatus=OldStatus&(~(SRWF_Many|SRWF_Hold));
		//有线程在操作链表，让它去唤醒吧
		if (OldStatus&SRWF_Link)
		{
			CurrStatus=InterlockedCompareExchange((volatile long*)SRWLock,NewStatus,OldStatus);
			if (CurrStatus==OldStatus)
				return ;
		}
		else
		{
			NewStatus|=SRWF_Link;
			CurrStatus=InterlockedCompareExchange((volatile long*)SRWLock,NewStatus,OldStatus);
			if (CurrStatus==OldStatus)
			{
				RtlpWakeSRWLock(SRWLock,NewStatus);
				return ;
			}
		}
		OldStatus=CurrStatus;
	}
}

BOOL NTAPI RtlTryAcquireSRWLockExclusive(RTL_SRWLOCK* SRWLock)
{
	BOOL IsLocked=InterlockedBitTestAndSet((LONG*)SRWLock,SRW_HOLD_BIT);
	return !(IsLocked==TRUE);
}

BOOL NTAPI RtlTryAcquireSRWLockShared(RTL_SRWLOCK* SRWLock)
{
	DWORD dwBackOffCount=0;
	SYNCSTATUS NewStatus;
	SYNCSTATUS CurrStatus;
	SYNCSTATUS OldStatus=InterlockedCompareExchange((volatile long*)SRWLock,(1<<SRW_COUNT_BIT)|SRWF_Hold,0);
	if (OldStatus==0)
		return TRUE;
	while (1) 
	{
		if ((OldStatus&SRWF_Hold) && ((OldStatus&SRWF_Wait) || (OldStatus&SRWM_ITEM)==(SYNCSTATUS)NULL))
			return FALSE;		
		if (OldStatus&SRWF_Wait)
			NewStatus=OldStatus+SRWF_Hold;
		else
			NewStatus=OldStatus+(1<<SRW_COUNT_BIT);
		CurrStatus=InterlockedCompareExchange((volatile long*)SRWLock,NewStatus,OldStatus);
		if (CurrStatus==OldStatus)
			return TRUE;
		RtlBackoff(&dwBackOffCount);
		OldStatus=(SYNCSTATUS)SRWLock->Ptr;
	}
}

BOOL NTAPI RtlpWaitCouldDeadlock()
{
	//byte_77F978A8极有可能是LdrpShutdownInProgress
	//进程退出时，各种资源即将被销毁，继续等待将会出现错误的结果
	return RtlDllShutdownInProgress()!=0;
}

//New ConditionVariable API
void NTAPI RtlpInitConditionVariable(PEB* pPeb)
{
	if (pPeb->NumberOfProcessors==1)
		ConditionVariableSpinCount=0; 
}
/***********************************************************************
 *           RtlInitializeConditionVariable   (NTDLL.@)
 *
 * Initializes the condition variable with NULL.
 *
 * PARAMS
 *  variable [O] condition variable
 *
 * RETURNS
 *  Nothing.
 */
void NTAPI RtlInitializeConditionVariable( RTL_CONDITION_VARIABLE *variable )
{
    variable->Ptr = NULL;
}

//将等待块插入 SRWLock 中
static BOOL __fastcall RtlpQueueWaitBlockToSRWLock(YY_CV_WAIT_BLOCK* pBolck, RTL_SRWLOCK *SRWLock, ULONG SRWLockMark)
{
	size_t shareCount;
	size_t Current;
	size_t New;
	ULONG backoff;
				
	for (;;)
	{
		Current = *(volatile long*)SRWLock;

		if ((Current & 0x1) == 0)
			break;


		if (SRWLockMark == 0)
		{
			pBolck->flag |= 0x1;
		}
		else if ((Current & 0x2) == 0 && YY_SRWLOCK_GET_BLOCK(Current))
		{
			return FALSE;
		}

		pBolck->next = NULL;					

		if (Current & 0x2)
		{
			pBolck->notify = NULL;
			pBolck->shareCount = 0;

			//_YY_CV_WAIT_BLOCK 结构体跟 _YY_SRWLOCK_WAIT_BLOCK兼容，所以能这样强转
			pBolck->back = (YY_CV_WAIT_BLOCK*)YY_SRWLOCK_GET_BLOCK(Current);

			New = (size_t)(pBolck) | (Current & YY_CV_MASK);
		}
		else
		{
			shareCount = Current >> 4;

			pBolck->shareCount = shareCount;
			pBolck->notify = pBolck;
			New = shareCount <= 1 ? (size_t)(pBolck) | 0x3 : (size_t)(pBolck) | 0xB;
		}

		//清泠 发现的Bug，我们应该返回 TRUE，减少必要的内核等待。
		if (InterlockedCompareExchange((volatile long*)SRWLock, New, Current) == Current)
			return TRUE;

		RtlBackoff(&backoff);
		//YieldProcessor();
	}

	return FALSE;
}

static void __fastcall RtlpWakeConditionVariable(RTL_CONDITION_VARIABLE *ConditionVariable, size_t ConditionVariableStatus, size_t WakeCount)
{
	//v16
	YY_CV_WAIT_BLOCK* notify = NULL;
	YY_CV_WAIT_BLOCK* pWake = NULL;
	YY_CV_WAIT_BLOCK* pWaitBlock;
	YY_CV_WAIT_BLOCK* pBlock;
	YY_CV_WAIT_BLOCK* tmp;
	YY_CV_WAIT_BLOCK* next;
	YY_CV_WAIT_BLOCK* back;
	YY_CV_WAIT_BLOCK** ppInsert = &pWake;
	size_t LastStatus;
	size_t MaxWakeCount;
	size_t Count = 0;

	for (;;)
	{
		pWaitBlock = YY_CV_GET_BLOCK(ConditionVariableStatus);

		if ((ConditionVariableStatus & 0x7) == 0x7)
		{
			ConditionVariableStatus = InterlockedExchange((volatile long*)ConditionVariable, 0);

			*ppInsert = YY_CV_GET_BLOCK(ConditionVariableStatus);

			break;
		}

		MaxWakeCount = WakeCount + (ConditionVariableStatus & 7);

		pBlock = pWaitBlock;

		for (; pBlock->notify == NULL;)
		{
			tmp = pBlock;
			pBlock = pBlock->back;
			pBlock->next = tmp;
		}

		if (MaxWakeCount <= Count)
		{
			LastStatus = InterlockedCompareExchange((volatile long*)ConditionVariable, (size_t)(pWaitBlock), ConditionVariableStatus);

			if (LastStatus == ConditionVariableStatus)
			{
				break;
			}

			ConditionVariableStatus = LastStatus;
		}
		else
		{
			notify = pBlock->notify;

			for (; MaxWakeCount > Count && notify->next;)
			{
				++Count;
				*ppInsert = notify;
				notify->back = NULL;

				next = notify->next;

				pWaitBlock->notify = next;
				next->back = NULL;

				ppInsert = &notify->back;

				notify = next;

			}

			if (MaxWakeCount <= Count)
			{
				LastStatus = InterlockedCompareExchange((volatile long*)ConditionVariable, (size_t)(pWaitBlock), ConditionVariableStatus);

				if (LastStatus == ConditionVariableStatus)
				{
					break;
				}

				ConditionVariableStatus = LastStatus;
			}
			else
			{
				LastStatus = InterlockedCompareExchange((volatile long*)ConditionVariable, 0, ConditionVariableStatus);


				if (LastStatus == ConditionVariableStatus)
				{
					*ppInsert = notify;
					notify->back = 0;

					break;
				}

				ConditionVariableStatus = LastStatus;
			}
		}
	}

	for (; pWake;)
	{
		back = pWake->back;

		if (!InterlockedBitTestAndReset((volatile LONG*)&pWake->flag, 1))
		{
			if (pWake->SRWLock == NULL || RtlpQueueWaitBlockToSRWLock(pWake, pWake->SRWLock, (pWake->flag >> 2) & 0x1) == FALSE)
			{
				NtReleaseKeyedEvent(GlobalKeyedEventHandle, pWake, 0, NULL);
			}
		}

		pWake = back;
	}

	return;
}

VOID
NTAPI
RtlWakeConditionVariable(
	_Inout_ RTL_CONDITION_VARIABLE *ConditionVariable
)
{
	size_t Current;
	size_t Last;

	Current = *(volatile long*)ConditionVariable;

	for (; Current; Current = Last)
	{
		if (Current & 0x8)
		{
			if ((Current & 0x7) == 0x7)
			{
				return;
			}

			Last = InterlockedCompareExchange((volatile long*)ConditionVariable, Current + 1, Current);
			if (Last == Current)
				return;
		}
		else
		{
			Last = InterlockedCompareExchange((volatile long*)ConditionVariable, Current | 0x8, Current);
			if (Last == Current)
			{
				RtlpWakeConditionVariable(ConditionVariable, Current + 8, 1);
				return;
			}
		}
	}
}
		
VOID
NTAPI
RtlWakeAllConditionVariable(
	_Inout_ RTL_CONDITION_VARIABLE *ConditionVariable
)
{
	size_t Current = *(volatile long*)ConditionVariable;
	size_t Last;
	YY_CV_WAIT_BLOCK* pBlock;
	YY_CV_WAIT_BLOCK* Tmp;

	for (; Current && (Current & 0x7) != 0x7; Current = Last)
	{
		if (Current & 0x8)
		{
			Last = InterlockedCompareExchange((volatile long*)ConditionVariable, Current | 0x7, Current);
			if (Last == Current)
				return;
		}
		else
		{
			Last = InterlockedCompareExchange((volatile long*)ConditionVariable, 0, Current);
			if (Last == Current)
			{

				for (pBlock = YY_CV_GET_BLOCK(Current); pBlock;)
				{
					Tmp = pBlock->back;

					if (!InterlockedBitTestAndReset((volatile LONG*)&pBlock->flag, 1))
					{
						NtReleaseKeyedEvent(GlobalKeyedEventHandle, pBlock, FALSE, NULL);
					}

					pBlock = Tmp;
				}

				return;
			}
		}
	}
}
		
static void __fastcall RtlpOptimizeConditionVariableWaitList(RTL_CONDITION_VARIABLE *ConditionVariable, size_t ConditionVariableStatus)
{
	YY_CV_WAIT_BLOCK *pWaitBlock;
	YY_CV_WAIT_BLOCK *pItem;
	YY_CV_WAIT_BLOCK *temp;
	size_t LastStatus;
				
	for (;;)
	{
		pWaitBlock = YY_CV_GET_BLOCK(ConditionVariableStatus);
		pItem = pWaitBlock;

		for (; pItem->notify == NULL;)
		{
			temp = pItem;
			pItem = pItem->back;
			pItem->next = temp;
		}

		pWaitBlock->notify = pItem->notify;

		LastStatus = InterlockedCompareExchange((volatile long*)ConditionVariable, (size_t)(pWaitBlock), ConditionVariableStatus);

		if (LastStatus == ConditionVariableStatus)
			break;

		ConditionVariableStatus = LastStatus;

		if (ConditionVariableStatus & 7)
		{
			RtlpWakeConditionVariable(ConditionVariable, ConditionVariableStatus, 0);
			return;
		}
	}
}

static BOOL __fastcall RtlpWakeSingle(RTL_CONDITION_VARIABLE *ConditionVariable, YY_CV_WAIT_BLOCK* pBlock)
{
	volatile long Current = *(volatile long*)ConditionVariable;
	YY_CV_WAIT_BLOCK *pWaitBlock;
	YY_CV_WAIT_BLOCK *pSuccessor;
	size_t Last;
	size_t New;
	size_t back;
	YY_CV_WAIT_BLOCK* notify;
	BOOL bRet;				

	for (; Current && (Current & 0x7) != 0x7;)
	{
		if (Current & 0x8)
		{
			Last = InterlockedCompareExchange((volatile long*)ConditionVariable, Current | 0x7, Current);

			if (Last == Current)
				return FALSE;

			Current = Last;
		}
		else
		{
			New = Current | 0x8;

			Last = InterlockedCompareExchange((volatile long*)ConditionVariable, New, Current);

			if (Last == Current)
			{
				Current = New;

				notify = NULL;
				bRet = FALSE;

				pWaitBlock = YY_CV_GET_BLOCK(Current);
				pSuccessor = pWaitBlock;

				if (pWaitBlock)
				{
					for (; pWaitBlock;)
					{
						if (pWaitBlock == pBlock)
						{
							if (notify)
							{
								pWaitBlock = pWaitBlock->back;
								bRet = TRUE;

								notify->back = pWaitBlock;

								if (!pWaitBlock)
									break;

								pWaitBlock->next = notify;
							}
							else
							{
								back = (size_t)(pWaitBlock->back);

								New = back == 0 ? back : back ^ ((New ^ back) & 0xF);

								Last = InterlockedCompareExchange((volatile long*)ConditionVariable, New, Current);

								if (Last == Current)
								{
									Current = New;
									if (back == 0)
										return TRUE;

									bRet = TRUE;
								}
								else
								{
									Current = Last;
								}

								pSuccessor = pWaitBlock = YY_CV_GET_BLOCK(Current);
								notify = NULL;
							}
						}
						else
						{
							pWaitBlock->next = notify;
							notify = pWaitBlock;
							pWaitBlock = pWaitBlock->back;
						}
					}

					if (pSuccessor)
						pSuccessor->notify = notify;
				}

				RtlpWakeConditionVariable(ConditionVariable, Current, 0);
				return bRet;
			}

			Current = Last;
		}
	}

	return FALSE;
}			

NTSTATUS
NTAPI
RtlSleepConditionVariableCS(
	_Inout_ RTL_CONDITION_VARIABLE *ConditionVariable,
	_Inout_ PRTL_CRITICAL_SECTION   CriticalSection,
	_In_    const LARGE_INTEGER *   dwMilliseconds
)
{
	YY_CV_WAIT_BLOCK StackWaitBlock;
	size_t OldConditionVariable;
	size_t NewConditionVariable;
	size_t LastConditionVariable;
	size_t SpinCount;
	NTSTATUS Status = STATUS_SUCCESS;

	StackWaitBlock.next = NULL;
	StackWaitBlock.flag = 2;
	StackWaitBlock.SRWLock = NULL;
	OldConditionVariable = *(size_t*)ConditionVariable;			

	for (;;)
	{
		NewConditionVariable = (size_t)(&StackWaitBlock) | (OldConditionVariable & YY_CV_MASK);
		StackWaitBlock.back = YY_CV_GET_BLOCK(OldConditionVariable);

		if (StackWaitBlock.back)
		{
			StackWaitBlock.notify = NULL;

			NewConditionVariable |= 0x8;
		}
		else
		{
			StackWaitBlock.notify = &StackWaitBlock;
		}

		LastConditionVariable = InterlockedCompareExchange((volatile long*)ConditionVariable, NewConditionVariable, OldConditionVariable);

		if (LastConditionVariable == OldConditionVariable)
			break;

		OldConditionVariable = LastConditionVariable;
	}

	RtlLeaveCriticalSection(CriticalSection);

	//0x8 标记新增时，才进行优化 ConditionVariableWaitList
	if ((OldConditionVariable ^ NewConditionVariable) & 0x8)
	{
		RtlpOptimizeConditionVariableWaitList(ConditionVariable, NewConditionVariable);
	}

	//自旋
	for (SpinCount = ConditionVariableSpinCount; SpinCount; --SpinCount)
	{
		if (!(StackWaitBlock.flag & 2))
			break;

		YieldProcessor();
	}			

	if (InterlockedBitTestAndReset((volatile LONG*)&StackWaitBlock.flag, 1))
	{	
		Status = NtWaitForKeyedEvent(GlobalKeyedEventHandle, (PVOID)&StackWaitBlock, 0, dwMilliseconds);

		if (Status == STATUS_TIMEOUT && RtlpWakeSingle(ConditionVariable, &StackWaitBlock) == FALSE)
		{
			NtWaitForKeyedEvent(GlobalKeyedEventHandle, (PVOID)&StackWaitBlock, 0, NULL);
			Status = STATUS_SUCCESS;
		}
	}

	RtlEnterCriticalSection(CriticalSection);

	return Status;
}	


NTSTATUS
NTAPI
RtlSleepConditionVariableSRW(
	_Inout_ RTL_CONDITION_VARIABLE *ConditionVariable,
	_Inout_ RTL_SRWLOCK *SRWLock,
	_In_ PLARGE_INTEGER dwMilliseconds,
	_In_ ULONG Flags
)
{
	size_t SpinCount;
	YY_CV_WAIT_BLOCK StackWaitBlock;			
	size_t Current;
	size_t New;
	size_t Last;
	NTSTATUS Status = STATUS_SUCCESS;

	if (Flags & ~RTL_CONDITION_VARIABLE_LOCKMODE_SHARED)
	{
		return STATUS_INVALID_PARAMETER_2;
	}

	StackWaitBlock.next = NULL;
	StackWaitBlock.flag = 2;
	StackWaitBlock.SRWLock = NULL;

	if (Flags& RTL_CONDITION_VARIABLE_LOCKMODE_SHARED)
	{
		StackWaitBlock.flag |= 0x4;
	}

	Current = *(volatile long*)ConditionVariable;

	for (;;)
	{
		New = (size_t)(&StackWaitBlock) | (Current & YY_CV_MASK);

		if (StackWaitBlock.back = YY_CV_GET_BLOCK(Current))
		{
			StackWaitBlock.notify = NULL;

			New |= 0x8;
		}
		else
		{
			StackWaitBlock.notify = &StackWaitBlock;
		}

		Last = InterlockedCompareExchange((volatile long*)ConditionVariable, New, Current);

		if (Last == Current)
		{
			break;
		}

		Current = Last;
	}

	if (Flags& RTL_CONDITION_VARIABLE_LOCKMODE_SHARED)
		RtlReleaseSRWLockShared(SRWLock);
	else
		RtlReleaseSRWLockExclusive(SRWLock);

	if ((Current ^ New) & 0x8)
	{
		//新增0x8 标记位才调用 RtlpOptimizeConditionVariableWaitList
		RtlpOptimizeConditionVariableWaitList(ConditionVariable, New);
	}

	//自旋
	for (SpinCount = ConditionVariableSpinCount; SpinCount; --SpinCount)
	{
		if (!(StackWaitBlock.flag & 2))
			break;

		YieldProcessor();
	}

	if (InterlockedBitTestAndReset((volatile LONG*)&StackWaitBlock.flag, 1))
	{
		Status = NtWaitForKeyedEvent(GlobalKeyedEventHandle, (PVOID)&StackWaitBlock, 0, dwMilliseconds);

		if (Status == STATUS_TIMEOUT && RtlpWakeSingle(ConditionVariable, &StackWaitBlock) == FALSE)
		{
			NtWaitForKeyedEvent(GlobalKeyedEventHandle, (PVOID)&StackWaitBlock, 0, NULL);
			Status = STATUS_SUCCESS;
		}
	}

	if (Flags& RTL_CONDITION_VARIABLE_LOCKMODE_SHARED)
		RtlAcquireSRWLockShared(SRWLock);
	else
		RtlAcquireSRWLockExclusive(SRWLock);

	return Status;
}

//通过延时来暂时退避竞争
void NTAPI RtlBackoff(DWORD* pCount)
{
	DWORD nBackCount=*pCount;
	if (nBackCount==0)
	{
		if (NtCurrentTeb()->ProcessEnvironmentBlock->NumberOfProcessors==1)
			return ;
		nBackCount=0x40;
		nBackCount*=2;
	}
	else
	{
		if (nBackCount<0x1FFF)
			nBackCount=nBackCount+nBackCount;
	}
	nBackCount=(__rdtsc()&(nBackCount-1))+nBackCount;
	//Win7原代码借用参数来计数，省去局部变量
	pCount=0;
	while ((DWORD)pCount<nBackCount)
	{
		YieldProcessor();
		(DWORD)pCount++;
	}
}