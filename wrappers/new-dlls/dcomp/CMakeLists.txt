
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(dcomp.dll dcomp.spec)

set(baseaddress_dcomp     0x613e0000)

list(APPEND SOURCE
    device.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/dcomp_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/dcomp.def)

add_library(dcomp SHARED ${SOURCE})
set_module_type(dcomp win32dll)
target_link_libraries(dcomp wine)
add_importlibs(dcomp advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET dcomp DESTINATION reactos/system32 FOR all)
