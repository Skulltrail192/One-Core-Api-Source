
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_definitions(
    -D__WINESRC__
    -D_SHLWAPI_)
	
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(shlwext.dll shlwext.spec)

list(APPEND SOURCE
    assoc.c
    main.c
    string.c
    thread.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/shlwext_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/shlwext.def)
	
set(baseaddress_shlwext     0x60050000)		

add_library(shlwext SHARED ${SOURCE})
set_module_type(shlwext win32dll)
target_link_libraries(shlwext wine uuid)
add_importlibs(shlwext shlwapi advapi32 msvcrt kernel32 user32 ole32 gdi32 ntdll userenv wininet shell32 propsys)
add_cd_file(TARGET shlwext DESTINATION reactos/system32 FOR all)
