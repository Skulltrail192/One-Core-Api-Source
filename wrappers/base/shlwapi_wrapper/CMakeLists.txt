
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_definitions(
    -D__WINESRC__
    -D_SHLWAPI_)
	
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(shlwapi_wrapper.dll shlwapi_wrapper.spec)

list(APPEND SOURCE
    assoc.c
    main.c
    string.c
    thread.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/shlwapi_wrapper_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/shlwapi_wrapper.def)
	
set(baseaddress_shlwapi_wrapper     0x60050000)		

add_library(shlwapi_wrapper SHARED ${SOURCE})
set_module_type(shlwapi_wrapper win32dll)
target_link_libraries(shlwapi_wrapper wine uuid)
add_importlibs(shlwapi_wrapper shlwapibase advapi32 msvcrt kernel32 user32 ole32 gdi32 ntdll userenv wininet shell32 propsys)
add_cd_file(TARGET shlwapi_wrapper DESTINATION reactos/system32 FOR all)
