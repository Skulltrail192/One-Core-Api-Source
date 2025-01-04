
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(uiautomationcoreext.dll uiautomationcoreext.spec)

set(baseaddress_uiautomationcoreext     0x613e0000)

list(APPEND SOURCE
    main.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/uiautomationcoreext_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/uiautomationcoreext.def)

add_library(uiautomationcoreext SHARED ${SOURCE})
set_module_type(uiautomationcoreext win32dll)
target_link_libraries(uiautomationcoreext wine)
add_importlibs(uiautomationcoreext dnsapi msvcrt kernel32 ntdll)
add_cd_file(TARGET uiautomationcoreext DESTINATION reactos/system32 FOR all)
