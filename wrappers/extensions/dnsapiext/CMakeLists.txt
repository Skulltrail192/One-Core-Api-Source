
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(dnsapiext.dll dnsapiext.spec)

set(baseaddress_dnsapiext     0x613e0000)

list(APPEND SOURCE
    dns.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/dnsapiext_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/dnsapiext.def)

add_library(dnsapiext SHARED ${SOURCE})
set_module_type(dnsapiext win32dll)
target_link_libraries(dnsapiext wine)
add_importlibs(dnsapiext dnsapi msvcrt kernel32 ntdll)
add_cd_file(TARGET dnsapiext DESTINATION reactos/system32 FOR all)
