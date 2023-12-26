
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(wldp.dll wldp.spec)

set(baseaddress_advapi32_wrapper     0x76520000)

list(APPEND SOURCE
    wldp.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/wldp_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/wldp.def)

add_library(wldp SHARED ${SOURCE})
set_module_type(wldp win32dll)
target_link_libraries(wldp wine)
add_importlibs(wldp msvcrt kernel32 ntdll)
add_cd_file(TARGET wldp DESTINATION reactos/system32 FOR all)
