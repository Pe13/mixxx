find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(PC_zix QUIET zix-0)
endif()

find_path(
        zix_INCLUDE_DIR
        NAMES zix/allocator.h
              zix/attributes.h
              zix/btree.h
              zix/bump_allocator.h
              zix/digest.h
              zix/environment.h
              zix/filesystem.h
              zix/hash.h
              zix/path.h
              zix/ring.h
              zix/sem.h
              zix/status.h
              zix/string_view.h
              zix/thread.h
              zix/tree.h
              zix/zix.h
        PATH_SUFFIXES zix-0
        HINTS ${PC_zix_INCLUDE_DIRS}
        DOC "zix include directory"
)
mark_as_advanced(zix_INCLUDE_DIR)

find_library(
        zix_LIBRARY
        NAMES zix-0 zix
        HINTS ${PC_zix_LIBRARY_DIRS}
        DOC "zix library"
)
mark_as_advanced(zix_LIBRARY)

if(DEFINED PC_zix_VERSION AND NOT PC_zix_VERSION STREQUAL "")
    set(zix_VERSION "${PC_zix_VERSION}")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
        zix
        REQUIRED_VARS zix_LIBRARY zix_INCLUDE_DIR
        VERSION_VAR zix_VERSION
)

if(zix_FOUND)
    set(zix_LIBRARIES "${zix_LIBRARY}")
    set(zix_INCLUDE_DIRS "${zix_INCLUDE_DIR}")
    set(zix_DEFINITIONS ${PC_zix_CFLAGS_OTHER})

    if(NOT TARGET zix::zix)
        add_library(zix::zix UNKNOWN IMPORTED)
        set_target_properties(
                zix::zix
                PROPERTIES
                IMPORTED_LOCATION "${zix_LIBRARY}"
                INTERFACE_COMPILE_OPTIONS "${PC_zix_CFLAGS_OTHER}"
                INTERFACE_INCLUDE_DIRECTORIES "${zix_INCLUDE_DIR}"
        )
    endif()
endif()

