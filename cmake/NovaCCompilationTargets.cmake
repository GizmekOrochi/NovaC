# Optional, separately installed Compilation asset. Core never depends on it.
if(NOT TARGET NovaC::Compilation)
    get_filename_component(_NOVAC_COMP_LIBDIR "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
    get_filename_component(_NOVAC_COMP_PREFIX "${_NOVAC_COMP_LIBDIR}/.." ABSOLUTE)
    add_library(NovaC::Compilation STATIC IMPORTED)
    set_target_properties(NovaC::Compilation PROPERTIES
        IMPORTED_LOCATION "${_NOVAC_COMP_LIBDIR}/libNovaCCompilation.a"
        INTERFACE_INCLUDE_DIRECTORIES "${_NOVAC_COMP_PREFIX}/include"
        INTERFACE_COMPILE_FEATURES "cxx_std_20"
        INTERFACE_LINK_LIBRARIES "NovaC::NovaC")
    unset(_NOVAC_COMP_LIBDIR)
    unset(_NOVAC_COMP_PREFIX)
endif()
