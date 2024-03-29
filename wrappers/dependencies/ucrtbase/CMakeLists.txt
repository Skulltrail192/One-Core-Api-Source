
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/crt/include)
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/crt/include/internal)
spec2def(ucrtbase.dll ucrtbase.spec ADD_IMPORTLIB)

add_definitions(
    -DUSE_MSVCRT_PREFIX
    -D_MSVCRT_
    -D_MSVCRT_LIB_
    -D_MT
    -D_CTYPE_DISABLE_MACROS
    -D_NO_INLINING
    -DCRTDLL
    -D__MINGW_IMPORT="")

list(APPEND SOURCE
	errno.c
	main.c
	strerror.c
	string.c
	wcs.c
    ucrtbase.rc
    ${CMAKE_CURRENT_BINARY_DIR}/ucrtbase_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/ucrtbase.def)

add_library(ucrtbase SHARED ${SOURCE})

set_module_type(ucrtbase win32dll ENTRYPOINT DllMain 12)
add_dependencies(ucrtbase psdk)
target_link_libraries(ucrtbase wine ${PSEH_LIB})

add_importlibs(ucrtbase kernel32 msvcrt ntdll)
add_cd_file(TARGET ucrtbase DESTINATION reactos/system32 FOR all)
