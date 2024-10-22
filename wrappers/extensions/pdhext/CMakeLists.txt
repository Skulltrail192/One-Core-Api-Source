
spec2def(pdhext.dll pdhext.spec)

list(APPEND SOURCE
    pdh_main.c
	version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/pdhext_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/pdhext.def)
	
set(baseaddress_pdhext     0x66320000)	

add_library(pdhext SHARED ${SOURCE})
set_module_type(pdhext win32dll ENTRYPOINT 0)
add_importlibs(pdhext pdh msvcrt kernel32 ntdll)
target_link_libraries(pdhext wine)
add_cd_file(TARGET pdhext DESTINATION reactos/system32 FOR all)

spec2def(pdhext.dll pdhext.spec)


