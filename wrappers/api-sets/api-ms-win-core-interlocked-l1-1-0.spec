@ stdcall -arch=i386 InterlockedDecrement(ptr) kernel32.InterlockedDecrement
@ stdcall -arch=i386 InterlockedCompareExchange(ptr long long) kernel32.InterlockedCompareExchange
@ stdcall -arch=i386 -ret64 InterlockedCompareExchange64(ptr double double)
@ stdcall -arch=i386 InterlockedExchange(ptr long) kernel32.InterlockedExchange
@ stdcall -arch=i386 InterlockedExchangeAdd(ptr long) kernel32.InterlockedExchangeAdd
@ stdcall -arch=i386 InterlockedIncrement(ptr) kernel32.InterlockedIncrement
@ stdcall InitializeSListHead(ptr) kernel32.InitializeSListHead
@ stdcall InterlockedFlushSList(ptr) kernel32.InterlockedFlushSList
@ stdcall InterlockedPopEntrySList(ptr) kernel32.InterlockedPopEntrySList
@ stdcall InterlockedPushEntrySList(ptr ptr) kernel32.InterlockedPushEntrySList
@ stdcall InterlockedPushListSList(ptr ptr ptr long) kernel32.InterlockedPushListSList
@ stdcall QueryDepthSList(ptr) kernel32.QueryDepthSList