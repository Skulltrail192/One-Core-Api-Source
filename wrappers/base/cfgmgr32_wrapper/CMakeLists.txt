
spec2def(cfgmgr32_wrapper.dll cfgmgr32_wrapper.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    cfgmgr32_wrapper.rc
    ${CMAKE_CURRENT_BINARY_DIR}/cfgmgr32_wrapper_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/cfgmgr32_wrapper.def)

add_library(cfgmgr32_wrapper MODULE ${SOURCE})

set_module_type(cfgmgr32_wrapper win32dll)
add_importlibs(cfgmgr32_wrapper setupapi msvcrt kernel32 ntdll)
add_cd_file(TARGET cfgmgr32_wrapper DESTINATION reactos/system32 FOR all)
