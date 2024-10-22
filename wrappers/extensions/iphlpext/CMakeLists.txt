
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine
					${REACTOS_SOURCE_DIR}/sdk/lib/tdilib)
spec2def(iphlpext.dll iphlpext.spec)

list(APPEND SOURCE
    iphlpapi.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/iphlpext_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/iphlpext.def)
	
if(ARCH STREQUAL "amd64")
	set(baseaddress_iphlpext     0x66CF0000)
elseif(ARCH STREQUAL "i386")
	set(baseaddress_iphlpext     0x6c3e0000)
endif()
	
add_library(iphlpext SHARED ${SOURCE})
set_module_type(iphlpext win32dll)
target_link_libraries(iphlpext wine tdilib)
add_importlibs(iphlpext advapi32 msvcrt kernel32 nsi iphlpapi ws2_32 ntdll)
add_cd_file(TARGET iphlpext DESTINATION reactos/system32 FOR all)
