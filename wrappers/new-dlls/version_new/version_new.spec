@ stdcall GetFileVersionInfoA(str long long ptr)
@ stdcall GetFileVersionInfoExA(long str long long ptr)
@ stdcall GetFileVersionInfoExW(long wstr long long ptr)
@ stdcall GetFileVersionInfoSizeA(str ptr)
@ stdcall GetFileVersionInfoSizeExA(long str ptr)
@ stdcall GetFileVersionInfoSizeExW(long wstr ptr)
@ stdcall GetFileVersionInfoSizeW(wstr ptr)
@ stdcall GetFileVersionInfoW(wstr long long ptr)
@ stdcall VerFindFileA(long str str str ptr ptr ptr ptr)
@ stdcall VerFindFileW(long wstr wstr wstr ptr ptr ptr ptr)
@ stdcall VerInstallFileA(long str str str str str ptr ptr)
@ stdcall VerInstallFileW(long wstr wstr wstr wstr wstr ptr ptr)
@ stdcall VerLanguageNameA(long str long) kernel32.VerLanguageNameA
@ stdcall VerLanguageNameW(long wstr long) kernel32.VerLanguageNameW
@ stdcall VerQueryValueA(ptr str ptr ptr)
@ stdcall VerQueryValueW(ptr wstr ptr ptr)

#@ stdcall -stub VerQueryValueIndexA(ptr str long ptr ptr ptr) versionbase.VerQueryValueIndexA
#@ stdcall -stub VerQueryValueIndexW(ptr wstr long ptr ptr ptr) version.VerQueryValueIndexW
