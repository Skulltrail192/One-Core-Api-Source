add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(cfgmgr32_wrapper.dll cfgmgr32_wrapper.spec)

list(APPEND SOURCE
	main.c
    cfgmgr32_wrapper.rc
    ${CMAKE_CURRENT_BINARY_DIR}/cfgmgr32_wrapper_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/cfgmgr32_wrapper.def)
	
set(baseaddress_cfgmgr32_wrapper     0x79e90000)	

add_library(cfgmgr32_wrapper SHARED ${SOURCE})
set_module_type(cfgmgr32_wrapper win32dll)
target_link_libraries(cfgmgr32_wrapper wine)
add_importlibs(cfgmgr32_wrapper setupapi msvcrt kernel32 ntdll)
add_cd_file(TARGET cfgmgr32_wrapper DESTINATION reactos/system32 FOR all)