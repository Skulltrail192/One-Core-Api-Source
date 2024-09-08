
spec2def(cryptex.dll cryptex.spec)

list(APPEND SOURCE
    main.c
	version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/cryptex_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/cryptex.def)
	
set(baseaddress_cryptex     0x6b270000)		

add_library(cryptex SHARED ${SOURCE})
set_module_type(cryptex win32dll ENTRYPOINT 0)
add_importlibs(cryptex crypt32 advapi32 msvcrt kernel32 ntdll)
target_link_libraries(cryptex wine)
add_cd_file(TARGET cryptex DESTINATION reactos/system32 FOR all)

spec2def(cryptex.dll cryptex.spec)


