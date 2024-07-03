
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(qwave.dll qwave.spec)

set(baseaddress_qwave 0x62020000)

list(APPEND SOURCE
    main.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/qwave_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/qwave.def)

add_library(qwave SHARED ${SOURCE})
set_module_type(qwave win32dll)
target_link_libraries(qwave wine)
add_importlibs(qwave advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET qwave DESTINATION reactos/system32 FOR all)
