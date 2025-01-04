
add_definitions(-D__WINESRC__)
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/include/psdk)
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/include/reactos/wine)

list(APPEND SOURCE
    CompatibilityTool.c
    resources.rc)

add_executable(wct ${SOURCE})
set_module_type(wct win32gui UNICODE ENTRYPOINT WinMain 16)
target_link_libraries(wct wine uuid)
add_importlibs(wct kernel32 comctl32 advapi32 gdi32 user32 ntdll msvcrt)

add_cd_file(TARGET wct DESTINATION reactos/system32 FOR all)