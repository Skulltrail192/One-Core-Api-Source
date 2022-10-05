
spec2def(cryptext_wrapper.dll cryptext_wrapper.spec)

list(APPEND SOURCE
    main.c
	version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/cryptext_wrapper_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/cryptext_wrapper.def)

add_library(cryptext_wrapper SHARED ${SOURCE})
set_module_type(cryptext_wrapper win32dll ENTRYPOINT 0)
add_importlibs(cryptext_wrapper pdhbase msvcrt kernel32 ntdll)
target_link_libraries(cryptext_wrapper wine)
add_cd_file(TARGET cryptext_wrapper DESTINATION reactos/system32 FOR all)

spec2def(cryptext_wrapper.dll cryptext_wrapper.spec)


