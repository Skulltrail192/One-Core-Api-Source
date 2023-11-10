
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(bcryptprimitives.dll bcryptprimitives.spec)

list(APPEND SOURCE
    main.c	
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/bcryptprimitives_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/bcryptprimitives.def)

add_library(bcryptprimitives SHARED ${SOURCE})
set_module_type(bcryptprimitives win32dll)
target_link_libraries(bcryptprimitives wine)
add_importlibs(bcryptprimitives advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET bcryptprimitives DESTINATION reactos/system32 FOR all)
