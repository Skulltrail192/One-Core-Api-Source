
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(netapiex.dll netapiex.spec)

list(APPEND SOURCE
    main.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/netapiex_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/netapiex.def)
	
set(baseaddress_netapiex     0x6aec0000)	

add_library(netapiex SHARED ${SOURCE})
set_module_type(netapiex win32dll)
target_link_libraries(netapiex wine)
add_importlibs(netapiex advapi32 msvcrt kernel32 netapi32 ntdll)
add_cd_file(TARGET netapiex DESTINATION reactos/system32 FOR all)
