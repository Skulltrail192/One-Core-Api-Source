@ stdcall AllocateUserPhysicalPages(long ptr ptr) kernelbase.AllocateUserPhysicalPages
@ stdcall AllocateUserPhysicalPagesNuma(ptr ptr ptr long) kernelbase.AllocateUserPhysicalPagesNuma
@ stdcall CreateFileMappingW(long ptr long long long wstr) kernelbase.CreateFileMappingW
@ stdcall FreeUserPhysicalPages(long ptr ptr) kernelbase.FreeUserPhysicalPages
@ stub GetMemoryErrorHandlingCapabilities ;(ptr) kernelbase.GetMemoryErrorHandlingCapabilities
@ stdcall MapViewOfFileEx(long long long long long ptr)
@ stdcall MapUserPhysicalPages(ptr long ptr) kernelbase.MapUserPhysicalPages
@ stub RegisterBadMemoryNotification
@ stdcall UnmapViewOfFile(ptr)
@ stub UnregisterBadMemoryNotification
@ stdcall VirtualAllocExNuma(ptr ptr long long long long) kernelbase.VirtualAllocExNuma
@ stdcall VirtualAlloc(ptr long long long) kernelbase.VirtualAlloc
@ stdcall VirtualAllocEx(long ptr long long long) kernelbase.VirtualAllocEx
@ stdcall VirtualFree(ptr long long) kernelbase.VirtualFree
@ stdcall VirtualFreeEx(long ptr long long) kernelbase.VirtualFreeEx
@ stdcall VirtualLock(ptr long) kernelbase.VirtualLock
@ stdcall VirtualProtect(ptr long long ptr) kernelbase.VirtualProtect
@ stdcall VirtualProtectEx(long ptr long long ptr) kernelbase.VirtualProtectEx
@ stdcall VirtualQuery(ptr ptr long) kernelbase.VirtualQuery
@ stdcall VirtualQueryEx(long ptr ptr long) kernelbase.VirtualQueryEx
@ stdcall VirtualUnlock(ptr long) kernelbase.VirtualUnlock
