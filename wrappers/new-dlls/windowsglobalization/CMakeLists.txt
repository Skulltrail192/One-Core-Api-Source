
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine
                    ${REACTOS_SOURCE_DIR}/wrappers/sdk/include/wsdk)
spec2def(windowsglobalization.dll windowsglobalization.spec ADD_IMPORTLIB)

set(baseaddress_windows_globalization 	0x613a0000)

list(APPEND SOURCE
    main.c	
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/windowsglobalization_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/windowsglobalization.def)

add_library(windowsglobalization SHARED ${SOURCE})
set_module_type(windowsglobalization win32dll)
target_link_libraries(windowsglobalization wine)
add_importlibs(windowsglobalization advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET windowsglobalization DESTINATION reactos/system32 FOR all)
