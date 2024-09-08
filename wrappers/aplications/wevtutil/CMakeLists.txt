
add_definitions(-D__WINESRC__)
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/include/psdk)
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/include/reactos/wine)

list(APPEND SOURCE
    main.c
    version.rc)

add_executable(wevtutil ${SOURCE})

set_module_type(wevtutil win32cui UNICODE)
target_link_libraries(wevtutil wine uuid)
add_importlibs(wevtutil kernel32 msvcrt ntdll)

add_cd_file(TARGET wevtutil DESTINATION reactos/system32 FOR all)