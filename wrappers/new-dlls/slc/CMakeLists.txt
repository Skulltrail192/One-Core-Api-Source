
add_definitions(-D__WINESRC__)
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(slc.dll slc.spec ADD_IMPORTLIB)

set(baseaddress_slc 0x638d0000)

list(APPEND SOURCE
    slc.c
	version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/slc_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/slc.def)
	
add_library(slc SHARED ${SOURCE})
set_module_type(slc win32dll)
target_link_libraries(slc wine uuid)
add_importlibs(slc shell32 gdi32 sppc oleaut32 ole32 user32 advapi32 shlwapi ntext kernel32 msvcrt)
add_cd_file(TARGET slc DESTINATION reactos/system32 FOR all)
