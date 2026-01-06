#.rst:
# FindFFMPEG
# ----------
#
# Try to find the required ffmpeg components (default: libavformat, libavutil, libavcodec)
#
# Next variables can be used to hint FFMPEG libs search:
#
# ::
#
#   PC_<component>_LIBRARY_DIRS
#   PC_FFMPEG_LIBRARY_DIRS
#   PC_<component>_INCLUDE_DIRS
#   PC_FFMPEG_INCLUDE_DIRS
#
# Once done this will define
#
# ::
#
#   FFMPEG_FOUND         - System has the all required components.
#   FFMPEG_INCLUDE_DIRS  - Include directory necessary for using the required components headers.
#   FFMPEG_LIBRARIES     - Link these to use the required ffmpeg components.
#   FFMPEG_DEFINITIONS   - Compiler switches required for using the required ffmpeg components.
#
# For each of the components it will additionally set.
#
# ::
#
#   libavcodec
#   libavdevice
#   libavformat
#   libavfilter
#   libavutil
#   libswscale
#   libswresample
#
# the following variables will be defined
#
# ::
#
#   <component>_FOUND        - System has <component>
#   <component>_INCLUDE_DIRS - Include directory necessary for using the <component> headers
#   <component>_LIBRARIES    - Link these to use <component>
#   <component>_DEFINITIONS  - Compiler switches required for using <component>
#   <component>_VERSION      - The components version
#
# the following import targets is created
#
# ::
#
#   FFMPEG::FFMPEG - for all components
#   FFMPEG::<component> - where <component> in lower case (FFMPEG::avcodec) for each components
#
# Copyright (c) 2006, Matthias Kretz, <kretz@kde.org>
# Copyright (c) 2008, Alexander Neundorf, <neundorf@kde.org>
# Copyright (c) 2011, Michael Jansen, <kde@michael-jansen.biz>
# Copyright (c) 2017, Alexander Drozdov, <adrozdoff@gmail.com>
# Copyright (c) 2019, Jan Holthuis, <holthuis.jan@googlemail.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

include(FindPackageHandleStandardArgs)

# The default components were taken from a survey over other FindFFMPEG.cmake files
if(NOT FFMPEG_FIND_COMPONENTS)
  set(FFMPEG_FIND_COMPONENTS libavcodec libavformat libavutil)
endif()

#
### Macro: find_component
#
# Checks for the given component by invoking pkgconfig and then looking up the libraries and
# include directories.
#
macro(find_component component pkgconfig library header)
  find_package(PkgConfig REQUIRED)
  if(PkgConfig_FOUND)
    pkg_check_modules(PC_FFMPEG_${component} ${pkgconfig})
  endif()

  find_path(
    FFMPEG_${component}_INCLUDE_DIRS
    ${header}
    HINTS
    ${PC_FFMPEG_${component}_INCLUDEDIR}
    ${PC_FFMPEG_${component}_INCLUDE_DIRS}
    ${PC_FFMPEG_INCLUDE_DIRS}
    PATH_SUFFIXES ffmpeg
  )

  # 1. Find the main component (e.g., libavcodec.a)
  find_library(
    FFMPEG_${component}_LIBRARY
    NAMES ${library}
    HINTS
    ${PC_FFMPEG_${component}_LIBDIR}
    ${PC_FFMPEG_${component}_LIBRARY_DIRS}
    ${PC_FFMPEG_LIBRARY_DIRS}
  )

  if(FFMPEG_${component}_LIBRARY)
    set(FFMPEG_${component}_FOUND TRUE)

    # Initialize the library list with the main component absolute path
    set(FFMPEG_${component}_LIBRARIES ${FFMPEG_${component}_LIBRARY})

    # Define system libs that should NEVER be resolved to absolute paths
    # (The linker will find these in the iOS SDK sysroot automatically)
    set(FFMPEG_SKIP_RESOLVE_LIBS
            System c m pthread dl c++ stdc++ objc iconv z resolv
    )

    # 2. Loop over dependencies provided by pkg-config
    foreach(_dep ${PC_FFMPEG_${component}_LIBRARIES})
      if(NOT _dep STREQUAL library)

        # Check if this is a system library
        if("${_dep}" IN_LIST FFMPEG_SKIP_RESOLVE_LIBS)
          # Pass it through as a name (CMake will treat it as -lName)
          list(APPEND FFMPEG_${component}_LIBRARIES ${_dep})
        else()
          # It's a custom lib (vpx, x264, etc.), resolve it to absolute path
          # to avoid missing -L flag issues.
          find_library(
            FFMPEG_DEP_${_dep}_LIBRARY
            NAMES ${_dep}
            HINTS ${PC_FFMPEG_${component}_LIBRARY_DIRS} ${PC_FFMPEG_LIBRARY_DIRS}
          )

          if(FFMPEG_DEP_${_dep}_LIBRARY)
            list(APPEND FFMPEG_${component}_LIBRARIES ${FFMPEG_DEP_${_dep}_LIBRARY})
          else()
            # If we can't find it (rare), fall back to the name
            list(APPEND FFMPEG_${component}_LIBRARIES ${_dep})
          endif()
        endif()

      endif()
    endforeach()

    # 3. Add Frameworks and other linker flags (e.g. -framework VideoToolbox)
    if(PC_FFMPEG_${component}_LDFLAGS_OTHER)
      list(APPEND FFMPEG_${component}_LIBRARIES ${PC_FFMPEG_${component}_LDFLAGS_OTHER})
    endif()

    message(STATUS "  - ${component} found: ${FFMPEG_${component}_LIBRARY}")
  else()
    message(STATUS "  - ${component} not found.")
  endif()

  set(
    FFMPEG_${component}_DEFINITIONS
    ${PC_FFMPEG_${component}_CFLAGS_OTHER}
    CACHE STRING
    "The ${component} CFLAGS."
  )
  set(
    FFMPEG_${component}_VERSION
    ${PC_FFMPEG_${component}_VERSION}
    CACHE STRING
    "The ${component} version number."
  )

  mark_as_advanced(
    FFMPEG_${component}_INCLUDE_DIRS
    FFMPEG_${component}_LIBRARY
    FFMPEG_${component}_LIBRARIES
    FFMPEG_${component}_DEFINITIONS
    FFMPEG_${component}_VERSION
  )
endmacro()

message(STATUS "Searching for FFMPEG components")
# Check for all possible component.
find_component(libavcodec    libavcodec    avcodec  libavcodec/avcodec.h)
find_component(libavformat   libavformat   avformat libavformat/avformat.h)
find_component(libavdevice   libavdevice   avdevice libavdevice/avdevice.h)
find_component(libavutil     libavutil     avutil   libavutil/avutil.h)
find_component(libavfilter   libavfilter   avfilter libavfilter/avfilter.h)
find_component(libswscale    libswscale    swscale  libswscale/swscale.h)
find_component(libswresample libswresample swresample libswresample/swresample.h)

set(FFMPEG_LIBRARIES "")
set(FFMPEG_DEFINITIONS "")
# Check if the required components were found and add their stuff to the FFMPEG_* vars.
foreach(component ${FFMPEG_FIND_COMPONENTS})
  if(FFMPEG_${component}_FOUND)
    message(STATUS "Required component ${component} present.")
    message(STATUS "${component} libraries: ${FFMPEG_${component}_LIBRARIES}")
    set(FFMPEG_LIBRARIES ${FFMPEG_LIBRARIES} ${FFMPEG_${component}_LIBRARIES})
    set(
      FFMPEG_DEFINITIONS
      ${FFMPEG_DEFINITIONS}
      ${FFMPEG_${component}_DEFINITIONS}
    )
    list(APPEND FFMPEG_INCLUDE_DIRS ${FFMPEG_${component}_INCLUDE_DIRS})
  endif()
endforeach()

# Build the include path with duplicates removed.
if(FFMPEG_INCLUDE_DIRS)
  list(REMOVE_DUPLICATES FFMPEG_INCLUDE_DIRS)
endif()

# cache the vars.
set(
  FFMPEG_INCLUDE_DIRS
  ${FFMPEG_INCLUDE_DIRS}
  CACHE STRING
  "The FFMPEG include directories."
  FORCE
)
set(
  FFMPEG_LIBRARIES
  ${FFMPEG_LIBRARIES}
  CACHE STRING
  "The FFMPEG libraries."
  FORCE
)
set(
  FFMPEG_DEFINITIONS
  ${FFMPEG_DEFINITIONS}
  CACHE STRING
  "The FFMPEG cflags."
  FORCE
)

mark_as_advanced(FFMPEG_INCLUDE_DIRS FFMPEG_LIBRARIES FFMPEG_DEFINITIONS)

# Compile the list of required vars
set(FFMPEG_REQUIRED_VARS FFMPEG_LIBRARIES FFMPEG_INCLUDE_DIRS)
foreach(component ${FFMPEG_FIND_COMPONENTS})
  list(
    APPEND
    FFMPEG_REQUIRED_VARS
    FFMPEG_${component}_LIBRARIES
    FFMPEG_${component}_INCLUDE_DIRS
  )
endforeach()

# Give a nice error message if some of the required vars are missing.
find_package_handle_standard_args(FFMPEG DEFAULT_MSG ${FFMPEG_REQUIRED_VARS})
