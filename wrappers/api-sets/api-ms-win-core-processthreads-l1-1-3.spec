#@ stub GetProcessDefaultCpuSets
@ stdcall GetProcessInformation(long long ptr long) kernelex.GetProcessInformation
@ stdcall GetSystemCpuSetInformation(ptr long ptr ptr long) kernelex.GetSystemCpuSetInformation
@ stdcall GetThreadDescription(long ptr) kernelex.GetThreadDescription
#@ stub GetThreadSelectedCpuSets
@ stdcall SetProcessDefaultCpuSets(ptr ptr long) kernelex.SetProcessDefaultCpuSets
@ stdcall SetProcessInformation(long long ptr long) kernelex.SetProcessInformation
@ stdcall SetThreadDescription(long ptr) kernelex.SetThreadDescription
@ stdcall SetThreadIdealProcessor(long long) kernel32.SetThreadIdealProcessor
@ stdcall SetThreadSelectedCpuSets(ptr ptr long) kernelex.SetThreadSelectedCpuSets
