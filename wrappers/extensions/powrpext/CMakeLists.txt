
spec2def(powrpext.dll powrpext.spec)

list(APPEND SOURCE
	main.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/powrpext_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/powrpext.def)

add_library(powrpext SHARED ${SOURCE})

set(baseaddress_powrpext     0x60020000)

set_module_type(powrpext win32dll ENTRYPOINT 0)
add_importlibs(powrpext advapi32 powrprof user32 comctl32 msvcrt kernel32 ntdll)
add_dependencies(powrpext psdk)
add_cd_file(TARGET powrpext DESTINATION reactos/system32 FOR all)
