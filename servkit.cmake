# library : servkit
# -- Version
set(servkit_maj_ver 0)
set(servkit_min_ver 0)
set(servkit_pat_ver 1)

set(servkit_lib_ver
    "${servkit_maj_ver}.${servkit_min_ver}.${servkit_pat_ver}")

set(servkit_lib_compat_ver
    "${servkit_maj_ver}.${servkit_min_ver}")

set(INCL include/servkit)
set(SRCS src/servkit)  

# -- Headers
# export
set(servkit_exp_hdr
    ${INCL}/asserts.h;
    ${INCL}/config.h;
    ${INCL}/connection.h;
    ${INCL}/likely.h;
    ${INCL}/net.h;
    )

# internal
set(servkit_int_hdr
    )

# -- Sources
set(servkit_src
    ${SRCS}/asserts.c;
    ${SRCS}/connection.c;
    ${SRCS}/net.c;
    )

# -- Targets
# --- shared
add_lib(servkit SHARED ${servkit_src} ${servkit_int_hdr} ${servkit_exp_hdr})
add_lib_build_def(servkit include/servkit/exportsym.h SK)
add_comp_def(servkit SERVKIT_MAJ=${servkit_maj_ver})
add_comp_def(servkit SERVKIT_MIN=${servkit_min_ver})
add_comp_def(servkit SERVKIT_PAT=${servkit_pat_ver})
add_comp_flag(servkit -pthread)
link_libs(servkit )
add_link_flag(servkit -pthread)
set_tgt_ver(servkit ${servkit_lib_ver} ${servkit_lib_compat_ver})

# --- static
#add_lib(servkit_s STATIC ${servkit_src} ${servkit_int_hdr} ${servkit_exp_hdr})
#add_lib_build_def(servkit_s include/servkit/exportsym.h SERVKIT)
#add_comp_def(servkit_s SERVKIT_MAJ=${servkit_maj_ver})
#add_comp_def(servkit_s SERVKIT_MIN=${servkit_min_ver})
#add_comp_def(servkit_s SERVKIT_PAT=${servkit_pat_ver})
#link_libs(servkit_s )
#set_tgt_ver(servkit_s ${servkit_lib_ver} ${servkit_lib_compat_ver})

# --- add catHttpServer
add_exe(catHttpServer catHttpServer/catHttpServer.c)
link_libs(catHttpServer servkit)

# -- Install!
install_hdr(${servkit_exp_hdr})
install_tgt(servkit)
#install_tgt(servkit_s)
