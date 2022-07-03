remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(${REACTOS_SOURCE_DIR}/sdk/include)
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/atl)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/psdk)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/sdk/include/psdk)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
include_directories(${REACTOS_SOURCE_DIR}/wrappers/dependencies/propsys)
include_directories(${REACTOS_SOURCE_DIR}/wrappers/dependencies/shellbase)
spec2def(shell32_wrapper.dll shell32_wrapper.spec)

list(APPEND SOURCE
	assoc.c
	classes.c
	debughlp.c
	iconcache.c
    main.c
	misc.c
	path.c
	pidl.c
	shellitem.c
	shellpath.c
	shlmenu.c
	shlview_cmenu.c
	unimplemented.c
	precomp.h
	version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/shell32_wrapper_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/shell32_wrapper.def)

	
add_library(shell32_wrapper SHARED ${SOURCE})

set_module_type(shell32_wrapper win32dll ENTRYPOINT 0)
target_link_libraries(shell32_wrapper wine uuid)
#add_pch(shell32_wrapper precomp.h SOURCE)
add_importlibs(shell32_wrapper advapi32 user32 msvcrt kernel32 ntdll shellbase shlwapi secur32 ole32 netapi32 rpcrt4 propsys comctl32 userenv gdi32 browseui version)
add_cd_file(TARGET shell32_wrapper DESTINATION reactos/system32 FOR all)
