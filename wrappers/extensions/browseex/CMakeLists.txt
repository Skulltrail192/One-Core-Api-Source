
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/host)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
include_directories(${REACTOS_SOURCE_DIR}/win32ss/user/winsrv/usersrv)

spec2def(browseex.dll browseex.spec)

set(baseaddress_browseex 	 0x69f90000)

list(APPEND SOURCE
	main.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/browseex_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/browseex.def)

add_library(browseex SHARED ${SOURCE})
set_module_type(browseex win32dll ENTRYPOINT 0)

target_link_libraries(browseex wine)

add_importlibs(browseex browseui kernel32 gdi32 ntdll advapi32)

add_cd_file(TARGET browseex DESTINATION reactos/system32 FOR all)
