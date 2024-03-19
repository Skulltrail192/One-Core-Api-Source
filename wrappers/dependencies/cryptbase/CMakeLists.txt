
remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x600 -DWINVER=0x600)

add_definitions(
    -D__WINESRC__
    -D__ROS_LONG64__
    -D_WINE
    -D_cryptbase_)

include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(cryptbase.dll cryptbase.spec ADD_IMPORTLIB)

if(MSVC)
    # error C4312: 'type cast': conversion from 'unsigned int' to 'void *' of greater size
    replace_compile_flags("/we4312" " ")
endif()

list(APPEND SOURCE
    base64.c
    cert.c
    chain.c
    collectionstore.c
    context.c
    crl.c
    ctl.c
    decode.c
    encode.c
    filestore.c
    main.c
    message.c
    msg.c
    object.c
    oid.c
    proplist.c
    protectdata.c
    provstore.c
    regstore.c
    rootstore.c
    serialize.c
    sip.c
    store.c
    str.c
    precomp.h)

add_library(cryptbase MODULE
    ${SOURCE}
    cryptbase.rc
    ${CMAKE_CURRENT_BINARY_DIR}/cryptbase_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/cryptbase.def)

set_module_type(cryptbase win32dll)
target_link_libraries(cryptbase wine ${PSEH_LIB} oldnames)
add_delay_importlibs(cryptbase cryptnet)
add_importlibs(cryptbase bcrypt user32 advapi32 advapi32_vista msvcrt kernel32 ntdll)
add_pch(cryptbase precomp.h SOURCE)
add_cd_file(TARGET cryptbase DESTINATION reactos/system32 FOR all)
