
add_definitions(-D_KERNEL32_)
add_definitions(-D__WINESRC__)

include_directories(
	${REACTOS_SOURCE_DIR}/sdk/lib/rtl
	${REACTOS_SOURCE_DIR}/sdk/include/crt
	${REACTOS_SOURCE_DIR}/sdk/include/ndk
	${REACTOS_SOURCE_DIR}/sdk/include/psdk
	${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine
	${REACTOS_SOURCE_DIR}/sdk/include/reactos/subsys
	${REACTOS_SOURCE_DIR}/sdk/include/reactos/subsys/win
	${REACTOS_SOURCE_DIR}/wrappers/sdk/include/wsdk)	
spec2def(kernelex.dll kernelex.spec ADD_IMPORTLIB)
spec2def(kernelbase.dll kernelbase.spec ADD_IMPORTLIB)

if(ARCH STREQUAL "amd64")
	set(baseaddress_kernelex   0x26D60000)
elseif(ARCH STREQUAL "i386")
	set(baseaddress_kernelex   0x68100000)
endif()

list(APPEND SOURCE
	ahcache.c
	appcompat.c
	appmodel.c
	baseinit.c
	console.c
	context.c
	curdir.c
	datetime.c	
	dep.c
	dir.c
	dllatom.c
	dllmain.c
	error.c
	filehops.c
	fileinfo.c
	filemap.c
	filemisc.c
	fileopcr.c	
	filefind.c
	firmware.c
	handle.c 
	heap.c
    locale.c
	mem.c
	module.c
	namepipe.c
	nls.c
	numa.c
	number.c
	path.c
	pathmisc.c
	perf.c
	powrmgmt.c
	process.c
	psapi.c
	reg.c
	security.c
	snapshot.c
	sortkey.c
	string.c
	support.c
	sxs.c
	syblink.c	
	synch.c
	sysinfo.c
	thread.c
	tick.c
	time.c	
	wer.c
	winlinks.c
	utils.c
	vdm.c
	winnls/ansi.c
    version.c
	xstate.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/kernelex_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/kernelex.def)
	
if(ARCH STREQUAL "i386")
    list(APPEND ASM_SOURCE
		# asm/i386/fiber.S
		# asm/i386/thread.S
		asm/i386/thunk.S
	)
elseif(ARCH STREQUAL "amd64")
    list(APPEND ASM_SOURCE
        # asm/amd64/fiber.S	
        # asm/amd64/thread.S	
	)
endif()	
	
add_asm_files(kernelex_ASM ${ASM_SOURCE})	
	
add_library(kernelex SHARED ${kernelex_ASM} ${SOURCE})

set_module_type(kernelex win32dll ENTRYPOINT BaseDllInitialize 12)

#############################################
## HACK FOR MSVC COMPILATION WITH win32dll ##
set_subsystem(kernelex console)
################# END  HACK #################

target_link_libraries(kernelex wine ${PSEH_LIB})

if(ARCH STREQUAL "i386")
	add_importlibs(kernelex kernel32 ntext advapi32 advapibase user32 shlwapi idndl)

elseif(ARCH STREQUAL "amd64")
	add_importlibs(kernelex kernel32 ntext advapi32 advapibase user32 shlwapi idndl)
endif()

add_cd_file(TARGET kernelex DESTINATION reactos/system32 FOR all)
