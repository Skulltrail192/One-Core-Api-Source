
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(windows.ui.dll windows.ui.spec)

set(baseaddress_windows.ui     0x64000000)

list(APPEND SOURCE
    inputpane.c
    main.c
    uisettings.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/windows.ui.def)

add_library(windows.ui SHARED ${SOURCE})
set_module_type(windows.ui win32dll)
target_link_libraries(windows.ui wine)
add_importlibs(windows.ui advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET windows.ui DESTINATION reactos/system32 FOR all)
