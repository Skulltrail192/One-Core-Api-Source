
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(d3dcompiler_39.dll d3dcompiler_39.spec ADD_IMPORTLIB)

set(baseaddress_avrt     0x64000000)

list(APPEND SOURCE
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/d3dcompiler_39_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/d3dcompiler_39.def)

add_library(d3dcompiler_39 SHARED ${SOURCE})
set_module_type(d3dcompiler_39 win32dll)
target_link_libraries(d3dcompiler_39 wine)
add_importlibs(d3dcompiler_39 advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET d3dcompiler_39 DESTINATION reactos/system32 FOR all)
