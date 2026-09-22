if(NOT TARGET NovaC::NovaC)
    add_library(NovaC::NovaC STATIC IMPORTED)

    # Resolve paths relative to this package file so custom LIBDIR values
    # (for example lib64) remain relocatable after installation.
    get_filename_component(_NOVAC_LIBDIR "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
    get_filename_component(_NOVAC_PREFIX "${_NOVAC_LIBDIR}/.." ABSOLUTE)

    set_target_properties(NovaC::NovaC PROPERTIES
        IMPORTED_LOCATION "${_NOVAC_LIBDIR}/libNovaC.a"
        INTERFACE_INCLUDE_DIRECTORIES "${_NOVAC_PREFIX}/include"
        INTERFACE_COMPILE_FEATURES "cxx_std_20"
    )

    unset(_NOVAC_LIBDIR)
    unset(_NOVAC_PREFIX)
endif()
