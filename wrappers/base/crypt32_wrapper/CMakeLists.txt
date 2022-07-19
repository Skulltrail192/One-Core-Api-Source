
spec2def(crypt32_wrapper.dll crypt32_wrapper.spec)

list(APPEND SOURCE
    main.c
	version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/crypt32_wrapper_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/crypt32_wrapper.def)

add_library(crypt32_wrapper SHARED ${SOURCE})
set_module_type(crypt32_wrapper win32dll ENTRYPOINT 0)
add_importlibs(crypt32_wrapper pdhbase msvcrt kernel32 ntdll)
target_link_libraries(crypt32_wrapper wine)
add_cd_file(TARGET crypt32_wrapper DESTINATION reactos/system32 FOR all)

spec2def(crypt32_wrapper.dll crypt32_wrapper.spec)


