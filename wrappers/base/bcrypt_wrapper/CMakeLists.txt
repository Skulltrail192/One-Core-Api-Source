add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(bcrypt_wrapper.dll bcrypt_wrapper.spec)

set(baseaddress_bcrypt_wrapper 0x71ad0000)

list(APPEND SOURCE
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/bcrypt_wrapper_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/bcrypt_wrapper.def)

add_library(bcrypt_wrapper SHARED ${SOURCE})
set_module_type(bcrypt_wrapper win32dll)
target_link_libraries(bcrypt_wrapper wine)
add_importlibs(bcrypt_wrapper advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET bcrypt_wrapper DESTINATION reactos/system32 FOR all)