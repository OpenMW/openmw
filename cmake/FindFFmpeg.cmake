# vim: ts=2 sw=2
# - Try to find the required ffmpeg components
#
# This module accepts the following env variable
#  FFMPEG_HOME - Can be set to custom install path
#
# Once done this will define
#  FFmpeg_FOUND         - System has the all required components.
#  FFmpeg_INCLUDE_DIRS  - Include directory necessary for using the required components headers.
#  FFmpeg_LIBRARIES     - Link these to use the required ffmpeg components.
#  FFmpeg_DEFINITIONS   - Compiler switches required for using the required ffmpeg components.
#
# For each of the components it will additionaly set.
#   - AVCODEC
#   - AVDEVICE
#   - AVFORMAT
#   - AVUTIL
#   - POSTPROCESS
#   - SWSCALE
#   - SWRESAMPLE
# the following variables will be defined
#  FFmpeg_<component>_FOUND        - System has <component>
#  FFmpeg_<component>_INCLUDE_DIRS - Include directory necessary for using the <component> headers
#  FFmpeg_<component>_LIBRARIES    - Link these to use <component>
#  FFmpeg_<component>_DEFINITIONS  - Compiler switches required for using <component>
#  FFmpeg_<component>_VERSION      - The components version
#
# Copyright (c) 2006, Matthias Kretz, <kretz@kde.org>
# Copyright (c) 2008, Alexander Neundorf, <neundorf@kde.org>
# Copyright (c) 2011, Michael Jansen, <kde@michael-jansen.biz>
# Copyright (c) 2016, Roman Proskuryakov, <humbug@deeptown.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

include(LibFindMacros)
include(FindPackageHandleStandardArgs)

# Macro: _internal_find_component
# Checks for the given component by invoking pkgconfig etc.
macro(_internal_find_component _component _pkgconfig _library _header)
    set(_package_component FFmpeg_${_component})
    libfind_pkg_detect(${_package_component} ${_pkgconfig}
        FIND_PATH ${_header}
            HINTS $ENV{FFMPEG_HOME}
            PATH_SUFFIXES include ffmpeg
        FIND_LIBRARY ${_library}
            HINTS $ENV{FFMPEG_HOME}
            PATH_SUFFIXES lib
    )
    set(${_package_component}_DEFINITIONS  ${${_package_component}_PKGCONF_CFLAGS_OTHER})
    set(${_package_component}_VERSION      ${${_package_component}_PKGCONF_VERSION})
    libfind_process(${_package_component})
endmacro()


# setter for 'hashmap'
macro(hashmap_set _table _key) # ARGN
    set(${_table}_${_key} ${ARGN})
endmacro()

# check for key in 'hashmap'
macro(hashmap_exists _table _key _out_var)
    if (DEFINED ${_table}_${_key})
        set(${_out_var} TRUE)
    else()
        set(${_out_var} FALSE)
    endif()
endmacro()

# getter for 'hashmap'
macro(hashmap_get _table _key _out_var)
    set(${_out_var} ${${_table}_${_key}})
endmacro()


# fill 'hashmap' named find_args
hashmap_set(find_args AVCODEC  libavcodec  avcodec  libavcodec/avcodec.h)
hashmap_set(find_args AVFORMAT libavformat avformat libavformat/avformat.h)
hashmap_set(find_args AVDEVICE libavdevice avdevice libavdevice/avdevice.h)
hashmap_set(find_args AVUTIL   libavutil   avutil   libavutil/avutil.h)
hashmap_set(find_args SWSCALE  libswscale  swscale  libswscale/swscale.h)
hashmap_set(find_args POSTPROC libpostproc postproc libpostproc/postprocess.h)
hashmap_set(find_args SWRESAMPLE  libswresample  swresample  libswresample/swresample.h)
hashmap_set(find_args AVRESAMPLE  libavresample  avresample  libavresample/avresample.h)

# Check if the required components were found and add their stuff to the FFmpeg_* vars.
foreach (_component ${FFmpeg_FIND_COMPONENTS})
    hashmap_exists(find_args ${_component} _known_component)
    if (NOT _known_component)
        message(FATAL_ERROR "Unknown component '${_component}'")
    endif()
    hashmap_get(find_args ${_component} _component_find_args)
    _internal_find_component(${_component} ${_component_find_args})
    set(_package_component FFmpeg_${_component})
    if (${_package_component}_FOUND)
        list(APPEND FFmpeg_LIBRARIES ${${_package_component}_LIBRARIES})
        list(APPEND FFmpeg_INCLUDE_DIRS ${${_package_component}_INCLUDE_DIRS})
        list(APPEND FFmpeg_DEFINITIONS ${${_package_component}_DEFINITIONS})
    endif ()
endforeach ()

# Build the include path with duplicates removed.
if (FFmpeg_INCLUDE_DIRS)
    list(REMOVE_DUPLICATES FFmpeg_INCLUDE_DIRS)
endif()

FIND_PACKAGE_HANDLE_STANDARD_ARGS(FFmpeg
    FOUND_VAR FFmpeg_FOUND
    HANDLE_COMPONENTS
    REQUIRED_VARS
        FFmpeg_LIBRARIES
        FFmpeg_INCLUDE_DIRS
)
