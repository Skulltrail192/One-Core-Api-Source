
spec2def(userenv_wrapper.dll userenv_wrapper.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    desktop.c
    directory.c
    environment.c
    gpolicy.c
    misc.c
    profile.c
    registry.c
    setup.c
    sid.c
    userenv_wrapper.c)

list(APPEND PCH_SKIP_SOURCE
    ${CMAKE_CURRENT_BINARY_DIR}/userenv_wrapper_stubs.c)

add_library(userenv_wrapper MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    userenv_wrapper.rc
    ${CMAKE_CURRENT_BINARY_DIR}/userenv_wrapper.def)

set_module_type(userenv_wrapper win32dll)
target_link_libraries(userenv_wrapper uuid)
add_importlibs(userenv_wrapper advapi32 user32 msvcrt kernel32 ntdll)
add_pch(userenv_wrapper precomp.h "${PCH_SKIP_SOURCE}")
add_cd_file(TARGET userenv_wrapper DESTINATION reactos/system32 FOR all)
