
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(winhttpext.dll winhttpext.spec)

set(baseaddress_winhttpext     0x6BBB0000)	

list(APPEND SOURCE
    main.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/winhttpext_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/winhttpext.def)

add_library(winhttpext SHARED ${SOURCE})
set_module_type(winhttpext win32dll)
target_link_libraries(winhttpext wine)
add_importlibs(winhttpext winhttp msvcrt kernel32 ntdll)
add_cd_file(TARGET winhttpext DESTINATION reactos/system32 FOR all)
