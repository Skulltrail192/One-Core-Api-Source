
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(dxgiext.dll dxgiext.spec)

set(baseaddress_dxgiext     0x613e0000)

list(APPEND SOURCE
    factory.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/dxgiext_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/dxgiext.def)

add_library(dxgiext SHARED ${SOURCE})
set_module_type(dxgiext win32dll)
target_link_libraries(dxgiext wine)
add_importlibs(dxgiext dnsapi msvcrt kernel32 ntdll dxgi)
add_cd_file(TARGET dxgiext DESTINATION reactos/system32 FOR all)
