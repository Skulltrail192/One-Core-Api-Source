
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/psdk)

spec2def(dwmapi.dll dwmapi.spec ADD_IMPORTLIB)

set(baseaddress_dwmapi     0x70F60000)

list(APPEND SOURCE
    dwmapi_main.c
    ${CMAKE_CURRENT_BINARY_DIR}/dwmapi_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/dwmapi.def)

add_library(dwmapi SHARED ${SOURCE} version.rc)
set_module_type(dwmapi win32dll ENTRYPOINT 0)
target_link_libraries(dwmapi wine)
add_importlibs(dwmapi kernel32 ntdll user32)
add_cd_file(TARGET dwmapi DESTINATION reactos/system32 FOR all)
