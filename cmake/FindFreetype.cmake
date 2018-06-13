#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see https://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

# - Try to find FreeType
#
# This module accepts the following env variable
#  FREETYPE_DIR - Can be set to custom install path
#
# Once done, this will define
#
#  Freetype_FOUND - system has FreeType
#  Freetype_INCLUDE_DIRS - the FreeType include directories
#  Freetype_LIBRARIES - link these to use FreeType
#  Freetype_VERSION - version of FreeType
#
# libfreetype internals:
#
# ======================================
# new versions (2.5.2)
#
# file structure:
#     <prefix>/include/freetype2/ft2build.h
#     <prefix>/include/freetype2/freetype.h
# used as:
#     #include <ft2build.h>
#     #include <freetype.h>
# requires:
#     -I <prefix>/include/freetype2/
#
# ======================================
# old versions (2.4.8, 2.3.5)
#
# file structure:
#     <prefix>/include/ft2build.h
#     <prefix>/include/freetype2/freetype/freetype.h
# used as:
#     #include <ft2build.h>
#     #include <freetype/freetype.h>
# requires:
#     -I <prefix>/include/ -I <prefix>/include/freetype2/
#
# ======================================

include(LibFindMacros)

set(_REGULAR_INSTALL_PATHS
    /usr/X11R6
    /usr/local/X11R6
    /usr/local/X11
    /usr/freeware
    ENV GTKMM_BASEPATH
    [HKEY_CURRENT_USER\\SOFTWARE\\gtkmm\\2.4;Path]
    [HKEY_LOCAL_MACHINE\\SOFTWARE\\gtkmm\\2.4;Path]
)

libfind_pkg_detect(Freetype freetype2
    FIND_PATH ft2build.h
        HINTS $ENV{FREETYPE_DIR}
        PATHS ${_REGULAR_INSTALL_PATHS}
        PATH_SUFFIXES include freetype2
    FIND_LIBRARY freetype freetype2311 freetype239 freetype238 freetype235 freetype219
        HINTS $ENV{FREETYPE_DIR}
        PATHS ${_REGULAR_INSTALL_PATHS}
        PATH_SUFFIXES lib
)
find_path(Freetype_OLD_INCLUDE_DIR
    # in new versions of freetype old_include_dir equals to include_dir
    # see explanation above
    NAMES freetype/freetype.h freetype.h
    PATHS ${Freetype_INCLUDE_DIR}
    PATH_SUFFIXES freetype2
    NO_DEFAULT_PATH
)
libfind_version_n_header(Freetype
    NAMES freetype/freetype.h freetype.h
    PATHS Freetype_OLD_INCLUDE_DIR
    DEFINES FREETYPE_MAJOR FREETYPE_MINOR FREETYPE_PATCH
)

set(Freetype_PROCESS_INCLUDES Freetype_OLD_INCLUDE_DIR)
libfind_process(Freetype)

if (Freetype_INCLUDE_DIRS)
    list(REMOVE_DUPLICATES Freetype_INCLUDE_DIRS)
endif()
