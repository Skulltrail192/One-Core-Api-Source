
add_definitions(
    -D__WINESRC__
    -D_olebase_
    -DCOM_NO_WINDOWS_H
    -DENTRY_PREFIX=olebase_
    -DPROXY_CLSID=CLSID_PSFactoryBuffer
    -DWINE_REGISTER_DLL)
	
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/wine)	
include_directories(${CMAKE_CURRENT_BINARY_DIR})	

spec2def(combase.dll combase.spec ADD_IMPORTLIB)

set(baseaddress_combase     0x61000000)

generate_idl_iids(activation.idl dcomcombase.idl inspectable.idl hstring.idl restrictederrorinfo.idl roparameterizediid.idl)
add_idl_headers(combaseidl activation.idl dcomcombase.idl inspectable.idl hstring.idl restrictederrorinfo.idl roparameterizediid.idl)
add_rpcproxy_files(
    activation.idl
    dcomcombase.idl
	inspectable.idl
	hstring.idl
	restrictederrorinfo.idl
	roparameterizediid.idl)

list(APPEND SOURCE
	apartment.c
    main.c
	mta.c
	roapi.c
	string.c
    #marshal.c
    #rpc.c
	version.rc
	# ${CMAKE_CURRENT_BINARY_DIR}/dcom_i.c
    ${CMAKE_CURRENT_BINARY_DIR}/combase_stubs.c
	#${CMAKE_CURRENT_BINARY_DIR}/proxy.dlldata.c
    ${CMAKE_CURRENT_BINARY_DIR}/combase.def)

add_library(combase SHARED  ${SOURCE})
set_module_type(combase win32dll ENTRYPOINT 0)
target_link_libraries(combase wine)
add_importlibs(combase olebase rpcrt4 msvcrt advapi32 kernel32 kernelbase ntdll)
add_dependencies(combase combaseidl)
add_cd_file(TARGET combase DESTINATION reactos/system32 FOR all)
