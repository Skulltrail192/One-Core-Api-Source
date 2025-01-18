include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/drivers/bootvid)

spec2def(ndisext.sys ndisext.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    ndisext.c
    ndisext.rc
    ${CMAKE_CURRENT_BINARY_DIR}/ndisext.def)

add_library(ndisext SHARED ${SOURCE})

set_module_type(ndisext kernelmodedriver ENTRYPOINT DriverEntry)

add_importlibs(ndisext ntoskrnl hal)

add_cd_file(TARGET ndisext DESTINATION reactos/system32/drivers NO_CAB FOR all)
