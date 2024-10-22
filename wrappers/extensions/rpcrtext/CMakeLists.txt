
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(rpcrtext.dll rpcrtext.spec)

list(APPEND SOURCE
    main.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/rpcrtext_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/rpcrtext.def)
	
set(baseaddress_rpcrtext     0x60030000)		

add_library(rpcrtext SHARED ${SOURCE})
set_module_type(rpcrtext win32dll)
target_link_libraries(rpcrtext wine)
add_importlibs(rpcrtext rpcrt4 msvcrt kernel32 ntdll)
add_cd_file(TARGET rpcrtext DESTINATION reactos/system32 FOR all)
