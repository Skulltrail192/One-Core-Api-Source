add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(progwrp_wrapper.dll progwrp_wrapper.spec)

list(APPEND SOURCE
    main.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/progwrp_wrapper_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/progwrp_wrapper.def)
	
set(baseaddress_progwrp_wrapper     0x54AE0000)	

add_library(progwrp_wrapper SHARED ${SOURCE})
set_module_type(progwrp_wrapper win32dll ENTRYPOINT 0)
target_link_libraries(progwrp_wrapper wine)
add_importlibs(progwrp_wrapper setupapi ntext kernelbase user32 shlwapi idndl psapi ole32 shell32 iphlpapi uxtheme dxgi)
add_cd_file(TARGET progwrp_wrapper DESTINATION reactos/system32 FOR all)