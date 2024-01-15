
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(wevtapi.dll wevtapi.spec)

set(baseaddress_wevtapi 	0x613a0000)

list(APPEND SOURCE
    main.c	
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/wevtapi_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/wevtapi.def)

add_library(wevtapi SHARED ${SOURCE})
set_module_type(wevtapi win32dll)
target_link_libraries(wevtapi wine)
add_importlibs(wevtapi advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET wevtapi DESTINATION reactos/system32 FOR all)
