add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(bcryptext.dll bcryptext.spec)

set(baseaddress_bcryptext 0x61ad0000)

list(APPEND SOURCE
    bcrypt_main.c
    gnutls.c
	md2.c
	macos.c
	sha256.c
	sha512.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/bcryptext_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/bcryptext.def)

add_library(bcryptext SHARED ${SOURCE})
set_module_type(bcryptext win32dll)
target_link_libraries(bcryptext wine)
add_importlibs(bcryptext advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET bcryptext DESTINATION reactos/system32 FOR all)