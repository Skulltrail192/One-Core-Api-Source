/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Condition Variable Routines
 * PROGRAMMERS:       Thomas Weidenmueller <w3seek@reactos.com>
 *                    Stephan A. R�ger
 */

/* NOTE: This functionality can be optimized for releasing single
   threads or for releasing all waiting threads at once. This
   implementation is optimized for releasing a single thread at a time.
   It wakes up sleeping threads in FIFO order. */

/* INCLUDES ******************************************************************/

#include <main.h>
#include <config.h>
#include <port.h>

// typedef struct __declspec(align(16)) _YY_CV_WAIT_BLOCK
// {
	// struct _YY_CV_WAIT_BLOCK* back;
	// struct _YY_CV_WAIT_BLOCK* notify;
	// struct _YY_CV_WAIT_BLOCK* next;
	// volatile size_t    shareCount;
	// volatile size_t    flag;
	// volatile RTL_SRWLOCK *SRWLock;
// } YY_CV_WAIT_BLOCK;

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
void WINAPI RtlInitializeConditionVariable( RTL_CONDITION_VARIABLE *variable )
{
    variable->Ptr = NULL;
}

			//将等待块插入 SRWLock 中
			static BOOL __fastcall RtlpQueueWaitBlockToSRWLock(YY_CV_WAIT_BLOCK* pBolck, RTL_SRWLOCK *SRWLock, ULONG SRWLockMark)
			{
				size_t shareCount;
				size_t Current;
				size_t New;
				
				for (;;)
				{
					Current = *(volatile size_t*)SRWLock;

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
					if (InterlockedCompareExchange((volatile size_t*)SRWLock, New, Current) == Current)
						return TRUE;

					//RtlBackoff(&v7);
					YieldProcessor();
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
				HANDLE GlobalKeyedEventHandle;
				
				GlobalKeyedEventHandle = GetGlobalKeyedEventHandle();

				for (;;)
				{
					pWaitBlock = YY_CV_GET_BLOCK(ConditionVariableStatus);

					if ((ConditionVariableStatus & 0x7) == 0x7)
					{
						ConditionVariableStatus = InterlockedExchange((volatile size_t*)ConditionVariable, 0);

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
						LastStatus = InterlockedCompareExchange((volatile size_t*)ConditionVariable, (size_t)(pWaitBlock), ConditionVariableStatus);

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
							LastStatus = InterlockedCompareExchange((volatile size_t*)ConditionVariable, (size_t)(pWaitBlock), ConditionVariableStatus);

							if (LastStatus == ConditionVariableStatus)
							{
								break;
							}

							ConditionVariableStatus = LastStatus;
						}
						else
						{
							LastStatus = InterlockedCompareExchange((volatile size_t*)ConditionVariable, 0, ConditionVariableStatus);


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
		WINAPI
		RtlWakeConditionVariable(
			_Inout_ RTL_CONDITION_VARIABLE *ConditionVariable
			)
		{
			size_t Current;
			size_t Last;

			Current = *(volatile size_t*)ConditionVariable;

			for (; Current; Current = Last)
			{
				if (Current & 0x8)
				{
					if ((Current & 0x7) == 0x7)
					{
						return;
					}

					Last = InterlockedCompareExchange((volatile size_t*)ConditionVariable, Current + 1, Current);
					if (Last == Current)
						return;
				}
				else
				{
					Last = InterlockedCompareExchange((volatile size_t*)ConditionVariable, Current | 0x8, Current);
					if (Last == Current)
					{
						RtlpWakeConditionVariable(ConditionVariable, Current + 8, 1);
						return;
					}
				}
			}
		}
		
		VOID
		WINAPI
		RtlWakeAllConditionVariable(
			_Inout_ RTL_CONDITION_VARIABLE *ConditionVariable
			)
		{
			size_t Current = *(volatile size_t*)ConditionVariable;
			size_t Last;
			YY_CV_WAIT_BLOCK* pBlock;
			YY_CV_WAIT_BLOCK* Tmp;
			HANDLE GlobalKeyedEventHandle;

			for (; Current && (Current & 0x7) != 0x7; Current = Last)
			{
				if (Current & 0x8)
				{
					Last = InterlockedCompareExchange((volatile size_t*)ConditionVariable, Current | 0x7, Current);
					if (Last == Current)
						return;
				}
				else
				{
					Last = InterlockedCompareExchange((volatile size_t*)ConditionVariable, 0, Current);
					if (Last == Current)
					{
						GlobalKeyedEventHandle = GetGlobalKeyedEventHandle();

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

					LastStatus = InterlockedCompareExchange((volatile size_t*)ConditionVariable, (size_t)(pWaitBlock), ConditionVariableStatus);

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
				volatile size_t Current = *(volatile size_t*)ConditionVariable;
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
						Last = InterlockedCompareExchange((volatile size_t*)ConditionVariable, Current | 0x7, Current);

						if (Last == Current)
							return FALSE;

						Current = Last;
					}
					else
					{
						New = Current | 0x8;

						Last = InterlockedCompareExchange((volatile size_t*)ConditionVariable, New, Current);

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

											Last = InterlockedCompareExchange((volatile size_t*)ConditionVariable, New, Current);

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
		WINAPI
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
			HANDLE GlobalKeyedEventHandle;
			NTSTATUS Status = STATUS_SUCCESS;
			//LARGE_INTEGER TimeOut;

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

				LastConditionVariable = InterlockedCompareExchange((volatile size_t*)ConditionVariable, NewConditionVariable, OldConditionVariable);

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


			GlobalKeyedEventHandle = GetGlobalKeyedEventHandle();

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

			//RtlSetLastNTError(Status);

			return Status;
		}	


		NTSTATUS
		WINAPI
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
			HANDLE GlobalKeyedEventHandle;
			NTSTATUS Status = STATUS_SUCCESS;

			// do
			// {
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

				Current = *(volatile size_t*)ConditionVariable;

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

					Last = InterlockedCompareExchange((volatile size_t*)ConditionVariable, New, Current);

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

				GlobalKeyedEventHandle = GetGlobalKeyedEventHandle();

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

			//} while (TRUE);


			//internal::BaseSetLastNTError(Status);

			return Status;

		}
/* EOF */
