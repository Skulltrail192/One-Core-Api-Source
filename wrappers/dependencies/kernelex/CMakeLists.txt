
add_definitions(-D_KERNEL32_)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/subsys)
spec2def(kernelex.dll kernelex.spec ADD_IMPORTLIB)

list(APPEND SOURCE
	stubs.c
    client/actctx.c
    client/appcache.c
    client/atom.c
    client/compname.c
    client/debugger.c
    client/dosdev.c
    client/dllmain.c
    client/environ.c
    client/except.c
    client/fiber.c
    client/handle.c
    client/heapmem.c
    client/job.c
    client/loader.c
    client/path.c
    client/perfcnt.c
    client/power.c
    client/proc.c
    client/resntfy.c
    client/session.c
    client/synch.c
    client/sysinfo.c
    client/time.c
    client/timerqueue.c
    client/toolhelp.c
    client/utils.c
    client/thread.c
    client/vdm.c
    client/version.c
    client/virtmem.c
    client/vista.c
    client/console/alias.c
    client/console/console.c
    client/console/history.c
    client/console/init.c
    client/console/readwrite.c
    client/console/vista.c
    client/file/backup.c
    client/file/cnotify.c
    client/file/copy.c
    client/file/create.c
    client/file/delete.c
    client/file/deviceio.c
    client/file/dir.c
    client/file/disk.c
    client/file/fileinfo.c
    client/file/filemap.c
    client/file/filename.c
    client/file/find.c
    client/file/hardlink.c
    client/file/iocompl.c
    client/file/lfile.c
    client/file/lock.c
    client/file/mailslot.c
    client/file/move.c
    client/file/mntpoint.c
    client/file/npipe.c
    client/file/rw.c
    client/file/tape.c
    client/file/volume.c
    wine/actctx.c
    wine/comm.c
    wine/lzexpand.c
    wine/muldiv.c
    wine/profile.c
    wine/res.c
    wine/timezone.c
    winnls/string/casemap.c
    winnls/string/collation.c
    winnls/string/fold.c
    winnls/string/format_msg.c
    winnls/string/lang.c
    winnls/string/lcformat.c
    winnls/string/lstring.c
    winnls/string/nls.c
    winnls/string/sortkey.c
	${CMAKE_CURRENT_BINARY_DIR}/kernelex_stubs.c
    k32.h)

if(ARCH STREQUAL "i386")
    list(APPEND ASM_SOURCE
        client/i386/fiber.S
        client/i386/thread.S)
elseif(ARCH STREQUAL "amd64")
    list(APPEND ASM_SOURCE
        client/amd64/fiber.S
        client/amd64/thread.S)
elseif(ARCH STREQUAL "arm")
    list(APPEND ASM_SOURCE
        client/arm/fiber.S
        client/arm/thread.S)
endif()

add_asm_files(kernelex_asm ${ASM_SOURCE})
add_library(kernelex SHARED
    ${SOURCE}
    ${kernelex_asm}
    kernelex.rc
    ${CMAKE_CURRENT_BINARY_DIR}/kernelex.def)

set_module_type(kernelex win32dll ENTRYPOINT DllMain 12)
#############################################
## HACK FOR MSVC COMPILATION WITH win32dll ##
set_subsystem(kernelex console)
################# END  HACK #################

target_link_libraries(kernelex wine chkstk ${PSEH_LIB})
add_importlibs(kernelex ntext ntdll)
add_pch(kernelex k32.h SOURCE)
add_dependencies(kernelex psdk errcodes asm)
add_cd_file(TARGET kernelex DESTINATION reactos/system32 FOR all)
