
add_definitions(-D_ADVAPI32_)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/rtl)
include_directories(${REACTOS_SOURCE_DIR}/dll/wrappers/ntdllnew)
include_directories(${REACTOS_SOURCE_DIR}/dll/win32/kernel32/include)
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/evtlib)

spec2def(advapiex.dll advapiex.spec ADD_IMPORTLIB)

set(baseaddress_advapiex     0x6c520000)

list(APPEND SOURCE
    cred.c
    efs.c
    eventlog.c
	hooks.c
	main.c
	perf.c
	process.c
	reg.c
	scm.c
	security.c
	sha1.c
	shutdown.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/advapiex_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/advapiex.def)

add_library(advapiex SHARED ${SOURCE})
set_module_type(advapiex win32dll ENTRYPOINT DllMain 12)

target_link_libraries(advapiex wine)

add_importlibs(advapiex advapi32 ntext kernel32 wevtapibase)

add_cd_file(TARGET advapiex DESTINATION reactos/system32 FOR all)
