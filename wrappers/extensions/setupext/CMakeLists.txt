
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(setupext.dll setupext.spec)

list(APPEND SOURCE
    devinst.c
    hooks.c
	query.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/setupext_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/setupext.def)
	
set(baseaddress_setupext     0x69370000)

add_library(setupext SHARED ${SOURCE})
set_module_type(setupext win32dll ENTRYPOINT 0)
target_link_libraries(setupext wine)
add_importlibs(setupext setupapi advapi32 kernel32 ntdll)
add_cd_file(TARGET setupext DESTINATION reactos/system32 FOR all)
