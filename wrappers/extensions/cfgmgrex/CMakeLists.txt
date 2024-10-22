add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(cfgmgrex.dll cfgmgrex.spec)

list(APPEND SOURCE
    main.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/cfgmgrex_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/cfgmgrex.def)
	
set(baseaddress_cfgmgrex     0x54AE0000)	

add_library(cfgmgrex SHARED ${SOURCE})
set_module_type(cfgmgrex win32dll ENTRYPOINT 0)
target_link_libraries(cfgmgrex wine)
add_importlibs(cfgmgrex setupapi)
add_cd_file(TARGET cfgmgrex DESTINATION reactos/system32 FOR all)