
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(userenv_wrapper.dll userenv_wrapper.spec)

list(APPEND SOURCE
    main.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/userenv_wrapper_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/userenv_wrapper.def)

add_library(userenv_wrapper SHARED ${SOURCE})
set_module_type(userenv_wrapper win32dll)
target_link_libraries(userenv_wrapper wine)
add_importlibs(userenv_wrapper userenvbase msvcrt kernel32 ntdll)
add_cd_file(TARGET userenv_wrapper DESTINATION reactos/system32 FOR all)
