
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(wdsmngqueue.dll wdsmngqueue.spec)

set(baseaddress_wdsmngqueue     0x67370000)

list(APPEND SOURCE
	main.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/wdsmngqueue_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/wdsmngqueue.def)

add_library(wdsmngqueue SHARED ${SOURCE})
set_module_type(wdsmngqueue win32dll)
target_link_libraries(wdsmngqueue wine)
add_importlibs(wdsmngqueue advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET wdsmngqueue DESTINATION reactos/system32 FOR all)
