
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(winstaext.dll winstaext.spec)

list(APPEND SOURCE
    main.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/winstaext_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/winstaext.def)
	
set(baseaddress_winstaext     0x57fd0000)		

add_library(winstaext SHARED ${SOURCE})
set_module_type(winstaext win32dll ENTRYPOINT 0)
target_link_libraries(winstaext wine)
add_importlibs(winstaext advapi32 kernel32 ntdll winstabase winstanew)
add_cd_file(TARGET winstaext DESTINATION reactos/system32 FOR all)
