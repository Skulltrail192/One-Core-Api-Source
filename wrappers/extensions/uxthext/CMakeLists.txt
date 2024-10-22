
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(uxthext.dll uxthext.spec)

list(APPEND SOURCE
    hooks.c
    main.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/uxthext_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/uxthext.def)
	
set(baseaddress_uxthext     0x6aac0000)		

add_library(uxthext SHARED ${SOURCE})
set_module_type(uxthext win32dll ENTRYPOINT 0)
target_link_libraries(uxthext wine)
add_importlibs(uxthext user32 advapi32 gdi32 kernel32 ntdll uxtheme)
add_cd_file(TARGET uxthext DESTINATION reactos/system32 FOR all)
