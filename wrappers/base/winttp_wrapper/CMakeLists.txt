
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(winhttp_wrapper.dll winhttp_wrapper.spec)

set(baseaddress_winhttp_wrapper     0x6BBB0000)	

list(APPEND SOURCE
    main.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/winhttp_wrapper_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/winhttp_wrapper.def)

add_library(winhttp_wrapper SHARED ${SOURCE})
set_module_type(winhttp_wrapper win32dll)
target_link_libraries(winhttp_wrapper wine)
add_importlibs(winhttp_wrapper winhttpbase msvcrt kernel32 ntdll)
add_cd_file(TARGET winhttp_wrapper DESTINATION reactos/system32 FOR all)
