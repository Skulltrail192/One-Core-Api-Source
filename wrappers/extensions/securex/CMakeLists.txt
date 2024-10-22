
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(securex.dll securex.spec)

list(APPEND SOURCE
    main.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/securex_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/securex.def)
	
set(baseaddress_securex     0x63050000)		

add_library(securex SHARED ${SOURCE})
set_module_type(securex win32dll)
target_link_libraries(securex wine)
add_importlibs(securex advapi32 msvcrt kernel32 ntdll securbase)
add_delay_importlibs(securex sspicli securnew)
add_cd_file(TARGET securex DESTINATION reactos/system32 FOR all)
