@ stdcall CreateFileMappingFromApp(long ptr long int64 wstr) kernelex.CreateFileMappingFromApp
@ stdcall CreateFileMappingNumaW(ptr ptr long long long wstr long) kernel32.CreateFileMappingNumaW
@ stdcall CreateFileMappingW(long ptr long long long wstr) kernel32.CreateFileMappingW
@ stdcall CreateMemoryResourceNotification(long) kernel32.CreateMemoryResourceNotification
@ stdcall FlushViewOfFile(ptr long) kernel32.FlushViewOfFile
@ stdcall GetLargePageMinimum() kernel32.GetLargePageMinimum
@ stdcall GetProcessWorkingSetSizeEx(long ptr ptr long) kernel32.GetProcessWorkingSetSizeEx
@ stdcall GetSystemFileCacheSize(ptr ptr ptr) kernel32.GetSystemFileCacheSize
@ stdcall GetWriteWatch(long ptr long ptr ptr ptr) kernel32.GetWriteWatch
@ stdcall MapViewOfFile(long long long long long) kernel32.MapViewOfFile
@ stdcall MapViewOfFileEx(long long long long long ptr) kernel32.MapViewOfFileEx
@ stdcall MapViewOfFileFromApp(long long int64 long) kernelex.MapViewOfFileFromApp
@ stdcall OpenFileMappingW(long long wstr) kernel32.OpenFileMappingW
@ stdcall PrefetchVirtualMemory(ptr ptr ptr long) kernelex.PrefetchVirtualMemory
@ stdcall QueryMemoryResourceNotification(ptr ptr) kernel32.QueryMemoryResourceNotification
@ stdcall ReadProcessMemory(long ptr ptr long ptr) kernel32.ReadProcessMemory
@ stdcall ResetWriteWatch(ptr long) kernel32.ResetWriteWatch
@ stdcall SetProcessWorkingSetSizeEx(long long long long) kernel32.SetProcessWorkingSetSizeEx
@ stdcall SetSystemFileCacheSize(long long long) kernel32.SetSystemFileCacheSize
@ stdcall UnmapViewOfFile(ptr) kernel32.UnmapViewOfFile
@ stdcall UnmapViewOfFileEx(ptr long) kernelex.UnmapViewOfFileEx
@ stdcall VirtualAlloc(ptr long long long) kernel32.VirtualAlloc
@ stdcall VirtualAllocEx(long ptr long long long) kernel32.VirtualAllocEx
@ stdcall VirtualFree(ptr long long) kernel32.VirtualFree
@ stdcall VirtualFreeEx(long ptr long long) kernel32.VirtualFreeEx
@ stdcall VirtualLock(ptr long) kernel32.VirtualLock
@ stdcall VirtualProtect(ptr long long ptr) kernel32.VirtualProtect
@ stdcall VirtualProtectEx(long ptr long long ptr) kernel32.VirtualProtectEx
@ stdcall VirtualQuery(ptr ptr long) kernel32.VirtualQuery
@ stdcall VirtualQueryEx(long ptr ptr long) kernel32.VirtualQueryEx
@ stdcall VirtualUnlock(ptr long) kernel32.VirtualUnlock
@ stdcall WriteProcessMemory(long ptr ptr long ptr) kernel32.WriteProcessMemory
