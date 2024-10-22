
spec2def(comdlgex.dll comdlgex.spec)

list(APPEND SOURCE
    cdlg32.c
    filedlg.c
    filedlg31.c
    filedlgbrowser.c
    itemdlg.c
    main.c
	comdlg32.rc
    ${CMAKE_CURRENT_BINARY_DIR}/comdlgex_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/comdlgex.def)
	
set(baseaddress_comdlgex     0x7f550000)	

add_library(comdlgex SHARED ${SOURCE})
set_module_type(comdlgex win32dll ENTRYPOINT DllMain)
add_importlibs(comdlgex comdlg32 user32 gdi32 advapi32 comctl32 shell32 shlwapi ole32 msvcrt kernel32 ntdll)
target_link_libraries(comdlgex uuid wine)
add_cd_file(TARGET comdlgex DESTINATION reactos/system32 FOR all)

spec2def(comdlgex.dll comdlgex.spec)


