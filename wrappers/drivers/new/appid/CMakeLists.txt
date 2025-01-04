include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/drivers/bootvid)

spec2def(appid.sys appid.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    appid.c
    appid.rc
    ${CMAKE_CURRENT_BINARY_DIR}/appid.def)

add_library(appid SHARED ${SOURCE})

set_module_type(appid kernelmodedriver ENTRYPOINT DriverEntry)

add_importlibs(appid ntoskrnl hal)

add_cd_file(TARGET appid DESTINATION reactos/system32/drivers NO_CAB FOR all)
