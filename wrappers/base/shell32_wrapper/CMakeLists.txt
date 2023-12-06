remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(${REACTOS_SOURCE_DIR}/sdk/include)
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/atl)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/psdk)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/sdk/include/psdk)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
include_directories(${REACTOS_SOURCE_DIR}/wrappers/dependencies/propsys)
include_directories(${REACTOS_SOURCE_DIR}/wrappers/dependencies/shellbase)
spec2def(shell32_wrapper.dll shell32_wrapper.spec)

set(baseaddress_shell32_wrapper     0x7b630000)

list(APPEND SOURCE
	assoc.c
	classes.c
	debughlp.c
	ebrowser.c
	iconcache.c
    main.c
	misc.c
	path.c
	pidl.c
	shellitem.c
	shelllink.c
	shellole.c
	shellpath.c
	shlfsbind.c
	shlfileop.c
	shlmenu.c
	#shlview.c
	shlview_cmenu.c
	unimplemented.c
	precomp.h
	version.rc
	${CMAKE_CURRENT_BINARY_DIR}/shell32_classes_i.c
	${CMAKE_CURRENT_BINARY_DIR}/shell32_classes_p.h
    ${CMAKE_CURRENT_BINARY_DIR}/shell32_wrapper_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/shell32_wrapper.def)

generate_idl_iids(shell32_classes.idl)
add_idl_headers(shell32_classes.idl)

add_rpcproxy_files(
    shell32_classes.idl)
	
add_library(shell32_wrapper SHARED ${SOURCE} ${CMAKE_CURRENT_BINARY_DIR}/shell32_classes_p.c)

set_module_type(shell32_wrapper win32dll ENTRYPOINT DllMain)
target_link_libraries(shell32_wrapper wine uuid)
#add_pch(shell32_wrapper precomp.h SOURCE)
add_importlibs(shell32_wrapper advapi32 user32 msvcrt kernel32 ntdll shellbase shlwapi secur32 ole32 netapi32 rpcrt4 propsys comctl32 userenv gdi32 browseui psapi version)
add_cd_file(TARGET shell32_wrapper DESTINATION reactos/system32 FOR all)
