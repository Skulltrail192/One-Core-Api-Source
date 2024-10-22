
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(userenvext.dll userenvext.spec)

list(APPEND SOURCE
    main.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/userenvext_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/userenvext.def)
	
	
set(baseaddress_userenvext     0x67920000)	

add_library(userenvext SHARED ${SOURCE})
set_module_type(userenvext win32dll)
target_link_libraries(userenvext wine)
add_importlibs(userenvext userenv msvcrt kernel32 ntdll)
add_cd_file(TARGET userenvext DESTINATION reactos/system32 FOR all)
