
add_definitions(
    -D__WINESRC__
    -D_olebase_
    -DCOM_NO_WINDOWS_H
    -DENTRY_PREFIX=olebase_
    -DPROXY_CLSID=CLSID_PSFactoryBuffer
    -DWINE_REGISTER_DLL)
	
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/wine)	

spec2def(combase.dll combase.spec)

#generate_idl_iids(dcom.idl)
#add_idl_headers(dcom.idl)

# add_rpcproxy_files(
    # dcom.idl)

list(APPEND SOURCE
	apartment.c
    main.c
	mta.c
    #marshal.c
    #rpc.c
	version.rc
	${CMAKE_CURRENT_BINARY_DIR}/dcom_i.c
    ${CMAKE_CURRENT_BINARY_DIR}/combase_stubs.c
	${CMAKE_CURRENT_BINARY_DIR}/proxy.dlldata.c
    ${CMAKE_CURRENT_BINARY_DIR}/combase.def)

add_library(combase SHARED  ${SOURCE} ${CMAKE_CURRENT_BINARY_DIR}/dcom_p.c)
set_module_type(combase win32dll ENTRYPOINT 0)
target_link_libraries(combase wine)
add_importlibs(combase olebase rpcrt4 msvcrt kernel32 ntdll)
add_cd_file(TARGET combase DESTINATION reactos/system32 FOR all)
