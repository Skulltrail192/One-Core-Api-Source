
add_definitions(-DLE)
spec2def(ws2_32_wrapper.dll ws2_32_wrapper.spec ADD_IMPORTLIB)

include_directories(
    inc
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/winsock)

list(APPEND SOURCE
    src/main.c
    src/protocol.c
    src/socket.c
    inc/ws2_32.h)

add_library(ws2_32_wrapper MODULE
    ${SOURCE}
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/ws2_32_wrapper.def)

set_module_type(ws2_32_wrapper win32dll)
target_link_libraries(ws2_32_wrapper ${PSEH_LIB} wine)
add_importlibs(ws2_32_wrapper user32 advapi32 ws2_base ws2help msvcrt normaliz kernel32 ntdll)
add_pch(ws2_32_wrapper inc/ws2_32.h SOURCE)
add_cd_file(TARGET ws2_32_wrapper DESTINATION reactos/system32 FOR all)
