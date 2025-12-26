find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(PC_serd QUIET serd-0)
endif()

find_path(
        serd_INCLUDE_DIR
        NAMES serd/serd.h
        PATH_SUFFIXES serd-0
        HINTS ${PC_serd_INCLUDE_DIRS}
        DOC "serd include directory"
)
mark_as_advanced(serd_INCLUDE_DIR)

find_library(
        serd_LIBRARY
        NAMES serd-0 serd
        HINTS ${PC_serd_LIBRARY_DIRS}
        DOC "serd library"
)
mark_as_advanced(serd_LIBRARY)

if(DEFINED PC_serd_VERSION AND NOT PC_serd_VERSION STREQUAL "")
    set(serd_VERSION "${PC_serd_VERSION}")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
        serd
        REQUIRED_VARS serd_LIBRARY serd_INCLUDE_DIR
        VERSION_VAR serd_VERSION
)

if(serd_FOUND)
    set(serd_LIBRARIES "${serd_LIBRARY}")
    set(serd_INCLUDE_DIRS "${serd_INCLUDE_DIR}")
    set(serd_DEFINITIONS ${PC_serd_CFLAGS_OTHER})

    if(NOT TARGET serd::serd)
        add_library(serd::serd UNKNOWN IMPORTED)
        set_target_properties(
                serd::serd
                PROPERTIES
                IMPORTED_LOCATION "${serd_LIBRARY}"
                INTERFACE_COMPILE_OPTIONS "${PC_serd_CFLAGS_OTHER}"
                INTERFACE_INCLUDE_DIRECTORIES "${serd_INCLUDE_DIR}"
        )
    endif()
endif()
