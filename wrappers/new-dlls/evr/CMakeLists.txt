
add_definitions(-D__WINESRC__)
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/dxsdk)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/psdk)

spec2def(evr.dll evr.spec ADD_IMPORTLIB)

set(baseaddress_evr     0x613d0000)

list(APPEND SOURCE
    evr.c
	dllmain.c
	main.c
	mixer.c
	presenter.c
	sample.c
    ${CMAKE_CURRENT_BINARY_DIR}/evr_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/evr.def)

add_idl_headers(evr_idlheader evr_classes.idl)
add_typelib(evr_classes.idl)
add_rpcproxy_files(evr_classes.idl)
	
add_library(evr SHARED ${SOURCE} evr.rc)
add_dependencies(evr evr_idlheader stdole2 dxsdk)
set_module_type(evr win32dll)
target_link_libraries(evr wine uuid mfuuid strmiids strmbase dxguid dxva2)
add_importlibs(evr shell32 oleaut32 ole32 user32 advapi32 shlwapi ntdll kernel32 msvcrt d3d9 mfplat)
add_cd_file(TARGET evr DESTINATION reactos/system32 FOR all)
