
add_definitions(
    -D__WINESRC__
    -D_WINE)

include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(comdlgbase.dll comdlgbase.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    cdlg32.c
    colordlg.c
    filedlg.c
    filedlg31.c
    filedlgbrowser.c
    finddlg.c
    fontdlg.c
    #itemdlg.c /* Win 7 */
    printdlg.c
    precomp.h
    ${CMAKE_CURRENT_BINARY_DIR}/comdlgbase_stubs.c)

add_library(comdlgbase MODULE
    ${SOURCE}
    rsrc.rc
    ${CMAKE_CURRENT_BINARY_DIR}/comdlgbase.def)

set_module_type(comdlgbase win32dll)
target_link_libraries(comdlgbase uuid wine)
add_delay_importlibs(comdlgbase ole32)
add_importlibs(comdlgbase shell32 shlwapi comctl32 winspool user32 gdi32 advapi32 msvcrt kernel32 ntdll)
add_pch(comdlgbase precomp.h SOURCE)
add_cd_file(TARGET comdlgbase DESTINATION reactos/system32 FOR all)
