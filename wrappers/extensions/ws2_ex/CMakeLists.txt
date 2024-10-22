
spec2def(ws2_ex.dll ws2_ex.spec)

include_directories(${REACTOS_SOURCE_DIR}/sdk/include/wine)

list(APPEND SOURCE
    main.c
	protocol.c
	socket.c
	version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/ws2_ex_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/ws2_ex.def)
	
set(baseaddress_ws2_ex     0x60070000)	

add_library(ws2_ex SHARED ${SOURCE})
set_module_type(ws2_ex win32dll ENTRYPOINT 0)
target_link_libraries(ws2_ex wine)
add_importlibs(ws2_ex ws2_32 msvcrt kernelex ntdll)
add_cd_file(TARGET ws2_ex DESTINATION reactos/system32 FOR all)
