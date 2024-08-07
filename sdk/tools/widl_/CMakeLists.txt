
if(MSVC)
    if(MSVC_VERSION LESS 1900)
        add_definitions(-Dsnprintf=_snprintf)

        # Add this definition for WDK only, VS 9 doesn't like that
        if(DEFINED ENV{DDKBUILDENV})
            add_definitions(-Dvsnprintf=_vsnprintf)
        endif()
    endif()
endif()

list(APPEND SOURCE
    attribute.c
    client.c
    expr.c
    hash.c
    header.c
    proxy.c
    register.c
    server.c
    typegen.c
    typelib.c
    typetree.c
    utils.c
    widl.c
    write_msft.c
    write_sltg.c
    ../port/getopt.c
    ../port/getopt1.c
    ../port/mkstemps.c)

# Taken from widl.rbuild
add_definitions(-DINT16=SHORT)
add_host_tool(widl ${SOURCE})
target_link_libraries(widl PRIVATE host_includes wpphost)

if(MSVC)
    # Disable warning "'>': signed/unsigned mismatch"
    target_compile_options(widl PRIVATE "/wd4018")

    # Disable warning "unary minus operator applied to unsigned type, result still unsigned"
    target_compile_options(widl PRIVATE "/wd4146")

    # Disable warning "'=': conversion from 'a' to 'b', possible loss of data"
    target_compile_options(widl PRIVATE "/wd4244")
endif()
