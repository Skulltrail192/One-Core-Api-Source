remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(${REACTOS_SOURCE_DIR}/sdk/include)
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/atl)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/psdk)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/sdk/include/psdk)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
include_directories(${REACTOS_SOURCE_DIR}/wrappers/dependencies/propsys)
include_directories(${REACTOS_SOURCE_DIR}/wrappers/dependencies/shellbase)
spec2def(shellex.dll shellex.spec)

set(baseaddress_shellex     0x6b630000)

list(APPEND SOURCE
	assoc.c
	classes.c
	debughlp.c
	ebrowser.c
	iconcache.c
    hooks.c
    main.c
	misc.c
	path.c
	pidl.c
	shellexec.c
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
    ${CMAKE_CURRENT_BINARY_DIR}/shellex_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/shellex.def)

generate_idl_iids(shell32_classes.idl)
add_idl_headers(shell32_classes.idl)

add_rpcproxy_files(
    shell32_classes.idl)
	
add_library(shellex SHARED ${SOURCE} ${CMAKE_CURRENT_BINARY_DIR}/shell32_classes_p.c)

set_module_type(shellex win32dll ENTRYPOINT DllMain)
target_link_libraries(shellex wine uuid)
#add_pch(shellex precomp.h SOURCE)
add_delay_importlibs(shellex propsys secur32 psapi)
add_importlibs(shellex advapi32 user32 msvcrt kernel32 ntdll shell32 shlwapi secur32 ole32 netapi32 rpcrt4 propsys comctl32 userenv gdi32 browseui psapi version)
add_cd_file(TARGET shellex DESTINATION reactos/system32 FOR all)
