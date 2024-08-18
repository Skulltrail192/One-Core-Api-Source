
spec2def(cryptex_wrapper.dll cryptex_wrapper.spec)

list(APPEND SOURCE
    main.c
	version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/cryptex_wrapper_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/cryptex_wrapper.def)
	
set(baseaddress_cryptex_wrapper     0x6b270000)		

add_library(cryptex_wrapper SHARED ${SOURCE})
set_module_type(cryptex_wrapper win32dll ENTRYPOINT 0)
add_importlibs(cryptex_wrapper crypt32 advapi32 msvcrt kernel32 ntdll)
target_link_libraries(cryptex_wrapper wine)
add_cd_file(TARGET cryptex_wrapper DESTINATION reactos/system32 FOR all)

spec2def(cryptex_wrapper.dll cryptex_wrapper.spec)


