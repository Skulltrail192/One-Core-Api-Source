/*
 * Copyright 2009 Henri Verbeet for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 */
#include <main.h>
#include <config.h>
#include <port.h>

			static void __fastcall RtlpWakeSRWLock(RTL_SRWLOCK *SRWLock, size_t Status)
			{
				YY_SRWLOCK_WAIT_BLOCK* pWatiBlock;
				YY_SRWLOCK_WAIT_BLOCK* notify;
				YY_SRWLOCK_WAIT_BLOCK* pBlock;
				YY_SRWLOCK_WAIT_BLOCK* back;
				YY_SRWLOCK_WAIT_BLOCK* next;
				volatile size_t NewStatus;
				HANDLE GlobalKeyedEventHandle;
				
				GlobalKeyedEventHandle = GetGlobalKeyedEventHandle();

				for (;;)
				{
					if ((Status & YY_SRWLOCK_Locked) == 0)
					{
						//微软就不判断下空指针？如此自信？
						pWatiBlock = YY_SRWLOCK_GET_BLOCK(Status);

						for (pBlock = pWatiBlock; (notify = pBlock->notify) == NULL;)
						{
							back = pBlock->back;
							back->next = pBlock;

							pBlock = back;
						}
						
						pWatiBlock->notify = notify;


						//判断是否是一个独占链
						if (notify->next && (notify->flag & 1))
						{
							pWatiBlock->notify = notify->next;
							notify->next = NULL;

							//SRWLock & (~YY_SRWLOCK_Waking)
#ifdef _WIN64
							_InterlockedAnd64((volatile LONG_PTR *)SRWLock, (LONG_PTR)(YY_SRWLOCK_Waking));
#else
							_InterlockedAnd((volatile LONG_PTR *)SRWLock, (LONG_PTR)(YY_SRWLOCK_Waking));
#endif
							if (!InterlockedBitTestAndReset((volatile LONG*)&notify->flag, 1))
							{
								//块处于等待状态，我们进行线程唤醒

								//if(!RtlpWaitCouldDeadlock())
								
								NtReleaseKeyedEvent(GlobalKeyedEventHandle, notify, 0, NULL);
							}

							return;
						}
						else
						{
							//等待的是一个共享锁，那么唤醒所有等待的共享锁。
							NewStatus = InterlockedCompareExchange((volatile size_t *)SRWLock, 0, Status);

							if (NewStatus == Status)
							{
								//更新成功！
								for (; notify;)
								{
									next = notify->next;


									if (!InterlockedBitTestAndReset((volatile LONG*)&notify->flag, 1))
									{
										//块处于等待状态，我们进行线程唤醒

										//if(!RtlpWaitCouldDeadlock())

										NtReleaseKeyedEvent(GlobalKeyedEventHandle, notify, 0, NULL);
									}

									notify = next;
								}

								return;
							}

							Status = NewStatus;
						}

						pWatiBlock->notify = notify;
					}
					else
					{
						NewStatus = InterlockedCompareExchange((volatile LONG *)SRWLock, Status & ~YY_SRWLOCK_Waking, Status);
						if (NewStatus == Status)
							return;

						Status = NewStatus;
					}
				}
			}
			
			static void __fastcall RtlpOptimizeSRWLockList(RTL_SRWLOCK* SRWLock, size_t Status)
			{
				size_t CurrentStatus;
				YY_SRWLOCK_WAIT_BLOCK* WatiBlock;
				YY_SRWLOCK_WAIT_BLOCK* back;
				YY_SRWLOCK_WAIT_BLOCK* pBlock;
				
				for (;;)
				{
					if (Status & YY_SRWLOCK_Locked)
					{
						if (WatiBlock = (YY_SRWLOCK_WAIT_BLOCK*)(Status & (~YY_SRWLOCK_MASK)))
						{
							pBlock = WatiBlock;

							for (; pBlock->notify == NULL;)
							{
								back = pBlock->back;
								back->next = pBlock;

								pBlock = back;
							}

							WatiBlock->notify = pBlock->notify;
						}

						//微软为什么用 Status - YY_SRWLOCK_Waking，而为什么不用 Status & ~YY_SRWLOCK_Waking ？
						CurrentStatus = InterlockedCompareExchange((volatile size_t *)SRWLock, Status - YY_SRWLOCK_Waking, Status);
						if (CurrentStatus == Status)
							break;

						Status = CurrentStatus;
					}
					else
					{
						RtlpWakeSRWLock(SRWLock, Status);
						break;
					}
				}
			}			
			
		VOID
		WINAPI
		RtlAcquireSRWLockExclusive(
			_Inout_ RTL_SRWLOCK *SRWLock
			)
		{
			YY_SRWLOCK_WAIT_BLOCK StackWaitBlock;
			BOOL bOptimize;
			volatile long OldBit;
			ULONG SpinCount;
			HANDLE GlobalKeyedEventHandle;
			size_t SRWLockOld;
			size_t SRWLockNew;

			//尝试加锁一次
#if defined(_WIN64)
			OldBit = InterlockedBitTestAndSet64((volatile LONG_PTR*)SRWLock, YY_SRWLOCK_Locked_BIT);
#else
			OldBit = InterlockedBitTestAndSet((volatile LONG_PTR*)SRWLock, YY_SRWLOCK_Locked_BIT);
#endif

			if(OldBit == FALSE)
			{
				//成功锁定
				return;
			}

			for (;;)
			{
				SRWLockOld =  *(volatile size_t*)SRWLock;

				if (YY_SRWLOCK_Locked & SRWLockOld)
				{
					/*
					if (RtlpWaitCouldDeadlock())
						ZwTerminateProcess((HANDLE)0xFFFFFFFF, 0xC000004B);
					*/

					StackWaitBlock.next = NULL;
					StackWaitBlock.flag = 3;
					bOptimize = FALSE;					

					if (YY_SRWLOCK_Waiting & SRWLockOld)
					{
						//有人正在等待连接
						StackWaitBlock.notify = NULL;
						StackWaitBlock.shareCount = 0;
						StackWaitBlock.back = (YY_SRWLOCK_WAIT_BLOCK*)(SRWLockOld & (~YY_SRWLOCK_MASK));

						SRWLockNew = (size_t)(&StackWaitBlock) | (SRWLockOld & YY_SRWLOCK_MultipleShared) | YY_SRWLOCK_Waking | YY_SRWLOCK_Waiting | YY_SRWLOCK_Locked;

						if ((YY_SRWLOCK_Waking & SRWLockOld) == 0)
						{
							bOptimize = TRUE;
						}
					}
					else
					{
						//没有其他人没有等待，所以我们需要创建一个
						StackWaitBlock.notify = (YY_SRWLOCK_WAIT_BLOCK*)&StackWaitBlock;
						StackWaitBlock.shareCount = (SRWLockOld >> YY_SRWLOCK_BITS);


						SRWLockNew = StackWaitBlock.shareCount > 1 ?
							(size_t)(&StackWaitBlock) | YY_SRWLOCK_MultipleShared | YY_SRWLOCK_Waiting | YY_SRWLOCK_Locked
							: (size_t)(&StackWaitBlock) | YY_SRWLOCK_Waiting | YY_SRWLOCK_Locked;
					}

					if (InterlockedCompareExchange((volatile size_t*)SRWLock, SRWLockNew, SRWLockOld) != SRWLockOld)
					{
						//更新锁状态失败，其他线程正在处理改锁，要不咋们换个姿势再来
				
						//RtlBackoff就懒得做了，反正只是等待一会而已，直接YieldProcessor再来一次吧。
						//RtlBackoff(&nBackOff)

						YieldProcessor();
						continue;
					}


					if (bOptimize)
					{
						RtlpOptimizeSRWLockList(SRWLock, SRWLockNew);
					}

					GlobalKeyedEventHandle = GetGlobalKeyedEventHandle();

					//自旋
					for (SpinCount = SRWLockSpinCount; SpinCount; --SpinCount)
					{
						if ((StackWaitBlock.flag & 2) == 0)
							break;

						YieldProcessor();
					}

					if (InterlockedBitTestAndReset((volatile LONG*)&StackWaitBlock.flag, 1))
					{
						NtWaitForKeyedEvent(GlobalKeyedEventHandle, (PVOID)&StackWaitBlock, 0, NULL);
					}
				}
				else
				{
					//尝试获取锁的所有权
					if (InterlockedCompareExchange((volatile size_t*)SRWLock, SRWLockOld | YY_SRWLOCK_Locked, SRWLockOld) == SRWLockOld)
					{
						//成功加锁
						return;
					}

					//可能多线程并发访问，换个姿势再来一次
					YieldProcessor();
				}
			}
		}		

		VOID
		WINAPI
		RtlReleaseSRWLockExclusive(
			_Inout_ RTL_SRWLOCK *SRWLock
			)
		{
			size_t OldSRWLock;
			size_t NewSRWLock;
			size_t CurrentSRWLock;
			
			OldSRWLock = InterlockedExchangeAdd((volatile size_t *)SRWLock, (size_t)-1);
			if ((OldSRWLock & YY_SRWLOCK_Locked) == 0)
			{
				RtlRaiseStatus(STATUS_RESOURCE_NOT_OWNED);
			}

			if ((OldSRWLock & YY_SRWLOCK_Waiting) && (OldSRWLock & YY_SRWLOCK_Waking) == 0)
			{
				OldSRWLock -= YY_SRWLOCK_Locked;

				NewSRWLock = OldSRWLock | YY_SRWLOCK_Waking;
				CurrentSRWLock = InterlockedCompareExchange((volatile size_t *)SRWLock, NewSRWLock, OldSRWLock);

				if (CurrentSRWLock == OldSRWLock)
					RtlpWakeSRWLock(SRWLock, NewSRWLock);
			}
		}
		
		BOOLEAN	
		WINAPI
		RtlTryAcquireSRWLockExclusive(
			_Inout_ RTL_SRWLOCK *SRWLock
			)
		{
#if defined(_WIN64)
			return InterlockedBitTestAndSet64((volatile LONG_PTR*)SRWLock, YY_SRWLOCK_Locked_BIT) == 0;
#else
			return InterlockedBitTestAndSet((volatile LONG_PTR*)SRWLock, YY_SRWLOCK_Locked_BIT) == 0;
#endif
		}
		
		VOID
		WINAPI
		RtlInitializeSRWLock(
			_Out_ RTL_SRWLOCK *SRWLock
			)
		{
			SRWLock->Ptr = NULL;
		}
		
		VOID
		WINAPI
		RtlReleaseSRWLockShared(
			_Inout_ RTL_SRWLOCK *SRWLock
			)
		{
			size_t OldSRWLock;
			size_t NewSRWLock;
			size_t LastSRWLock;
			YY_SRWLOCK_WAIT_BLOCK *pLastNode;
			//尝试解锁只加一次读锁的情况

			OldSRWLock = InterlockedCompareExchange((volatile size_t*)SRWLock, 0, (size_t)0x11);

			//解锁成功
			if (OldSRWLock == (size_t)0x11)
			{
				return;
			}

			if ((OldSRWLock & YY_SRWLOCK_Locked) == 0)
			{
				RtlRaiseStatus(STATUS_RESOURCE_NOT_OWNED);
			}

			for (;;)
			{
				if (OldSRWLock & YY_SRWLOCK_Waiting)
				{
					if (OldSRWLock & YY_SRWLOCK_MultipleShared)
					{
						pLastNode = YY_SRWLOCK_GET_BLOCK(OldSRWLock);

						for (; pLastNode->notify == NULL; pLastNode = pLastNode->back);

						/* 
						既然是在释放共享锁，说明一定有人获取了共享锁
						如果有人获取了共享锁，就一定没有人获取独到占锁
						只需要把共享次数减1
						取出notify节点的共享次数变量的地址, 原子减
						*/
						if (InterlockedDecrement((volatile size_t *)&(pLastNode->notify->shareCount)) > 0)
						{
							return;
						}
					}


					for (;;)
					{
						NewSRWLock = OldSRWLock & (~(YY_SRWLOCK_MultipleShared | YY_SRWLOCK_Locked));						

						if (OldSRWLock & YY_SRWLOCK_Waking)
						{
							LastSRWLock = InterlockedCompareExchange((volatile size_t *)SRWLock, NewSRWLock, OldSRWLock);

							if (LastSRWLock == OldSRWLock)
								return;
						}
						else
						{
							NewSRWLock |= YY_SRWLOCK_Waking;

							LastSRWLock = InterlockedCompareExchange((volatile size_t *)SRWLock, NewSRWLock, OldSRWLock);
							if (LastSRWLock == OldSRWLock)
								RtlpWakeSRWLock(SRWLock, NewSRWLock);
						}

						OldSRWLock = LastSRWLock;
					}

					break;
				}
				else
				{
					NewSRWLock = (size_t)YY_SRWLOCK_GET_BLOCK(OldSRWLock) <= 0x10 ? 0 : OldSRWLock - 0x10;

					LastSRWLock = InterlockedCompareExchange((volatile size_t *)SRWLock, NewSRWLock, OldSRWLock);
					if (LastSRWLock == OldSRWLock)
						break;

					OldSRWLock = LastSRWLock;
				}
			}
		}
		
		VOID
		WINAPI
		RtlAcquireSRWLockShared(
			_Inout_ RTL_SRWLOCK *SRWLock
			)
		{
			YY_SRWLOCK_WAIT_BLOCK StackWaitBlock;
			BOOL bOptimize;
			size_t NewSRWLock;
			size_t OldSRWLock;
			ULONG SpinCount;
			HANDLE GlobalKeyedEventHandle;

			//尝试给全新的锁加锁	
			OldSRWLock = InterlockedCompareExchange((volatile size_t*)SRWLock, (size_t)(0x11), 0);

			//成功
			if (OldSRWLock == (size_t)(0))
			{
				return;
			}			

			for (;; OldSRWLock = *(volatile size_t *)SRWLock)
			{
				if ((OldSRWLock & YY_SRWLOCK_Locked) && ((OldSRWLock & YY_SRWLOCK_Waiting) || YY_SRWLOCK_GET_BLOCK(OldSRWLock) == NULL))
				{
					//if ( RtlpWaitCouldDeadlock() )
					//    ZwTerminateProcess((HANDLE)0xFFFFFFFF, 0xC000004B);


					StackWaitBlock.flag = 2;
					StackWaitBlock.shareCount = 0;
					StackWaitBlock.next = NULL;

					bOptimize = FALSE;

					if (OldSRWLock & YY_SRWLOCK_Waiting)
					{
						//已经有人等待，我们插入一个新的等待块
						StackWaitBlock.back = YY_SRWLOCK_GET_BLOCK(OldSRWLock);
						StackWaitBlock.notify = NULL;

						NewSRWLock = (size_t)&StackWaitBlock | (OldSRWLock & YY_SRWLOCK_MultipleShared) | (YY_SRWLOCK_Waking | YY_SRWLOCK_Waiting | YY_SRWLOCK_Locked);

						if ((OldSRWLock & YY_SRWLOCK_Waking) == 0)
						{
							bOptimize = TRUE;
						}
					}
					else
					{
						StackWaitBlock.notify = &StackWaitBlock;
						NewSRWLock = (size_t)&StackWaitBlock | (YY_SRWLOCK_Waiting | YY_SRWLOCK_Locked);
					}


					if (InterlockedCompareExchange((volatile size_t *)SRWLock, NewSRWLock, OldSRWLock) == OldSRWLock)
					{
						//更新成功！

						if (bOptimize)
						{
							RtlpOptimizeSRWLockList(SRWLock, NewSRWLock);
						}

						GlobalKeyedEventHandle = GetGlobalKeyedEventHandle();
						//自旋
						for (SpinCount = SRWLockSpinCount; SpinCount; --SpinCount)
						{
							if ((StackWaitBlock.flag & 2) == 0)
								break;

							YieldProcessor();
						}

						if (InterlockedBitTestAndReset((volatile LONG*)&StackWaitBlock.flag, 1))
						{
							NtWaitForKeyedEvent(GlobalKeyedEventHandle, (PVOID)&StackWaitBlock, 0, NULL);
						}

						continue;
					}
				}
				else
				{
					if (OldSRWLock & YY_SRWLOCK_Waiting)
					{
						//既然有人在等待锁，那么YY_SRWLOCK_Locked应该重新加上
						NewSRWLock = OldSRWLock | YY_SRWLOCK_Locked;
					}
					else
					{
						//没有人等待，那么单纯加个 0x10即可
						NewSRWLock = (OldSRWLock + 0x10) | YY_SRWLOCK_Locked;
					}

					if (InterlockedCompareExchange((volatile size_t *)SRWLock, NewSRWLock, OldSRWLock) == OldSRWLock)
						return;
				}

				//偷懒下，直接 YieldProcessor 吧
				//RtlBackoff(&nBackOff);
				YieldProcessor();
			}
		}

		BOOLEAN
		WINAPI
		RtlTryAcquireSRWLockShared(
			_Inout_ RTL_SRWLOCK *SRWLock
			)
		{
			size_t NewSRWLock;
			size_t OldSRWLock;

			//尝试给全新的锁加锁
			OldSRWLock = InterlockedCompareExchange((volatile size_t*)SRWLock, (size_t)(0x11), 0);

			//成功
			if (OldSRWLock == (size_t)(0))
			{
				return TRUE;
			}

			for (;;)
			{
				if ((OldSRWLock & YY_SRWLOCK_Locked) && ((OldSRWLock & YY_SRWLOCK_Waiting) || YY_SRWLOCK_GET_BLOCK(OldSRWLock) == NULL))
				{
					//正在被锁定中
					return FALSE;
				}
				else
				{
					if (OldSRWLock & YY_SRWLOCK_Waiting)
						NewSRWLock = OldSRWLock | YY_SRWLOCK_Locked;
					else
						NewSRWLock = (OldSRWLock + 0x10) | YY_SRWLOCK_Locked;

					if (InterlockedCompareExchange((volatile size_t*)SRWLock, NewSRWLock, OldSRWLock) == OldSRWLock)
					{
						//锁定完成
						return TRUE;
					}

					//RtlBackoff((unsigned int *)&v4);
					YieldProcessor();
					OldSRWLock = *(volatile size_t*)SRWLock;
				}
			}
		}		