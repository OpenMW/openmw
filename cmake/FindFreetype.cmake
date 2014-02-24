#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

# - Try to find FreeType
# Once done, this will define
#
#  FREETYPE_FOUND - system has FreeType
#  FREETYPE_INCLUDE_DIRS - the FreeType include directories 
#  FREETYPE_LIBRARIES - link these to use FreeType

include(FindPkgMacros)
findpkg_begin(FREETYPE)

# Get path, convert backslashes as ${ENV_${var}}
getenv_path(FREETYPE_HOME)

# construct search paths
set(FREETYPE_PREFIX_PATH ${FREETYPE_HOME} ${ENV_FREETYPE_HOME})
create_search_paths(FREETYPE)
# redo search if prefix path changed
clear_if_changed(FREETYPE_PREFIX_PATH
  FREETYPE_LIBRARY_FWK
  FREETYPE_LIBRARY_REL
  FREETYPE_LIBRARY_DBG
  FREETYPE_INCLUDE_DIR
)

set(FREETYPE_LIBRARY_NAMES freetype2311 freetype239 freetype238 freetype235 freetype219 freetype)
get_debug_names(FREETYPE_LIBRARY_NAMES)

use_pkgconfig(FREETYPE_PKGC freetype2)

# prefer static library over framework 
set(CMAKE_FIND_FRAMEWORK "LAST")

message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")
findpkg_framework(FREETYPE)
message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")

find_path(FREETYPE_INCLUDE_DIR NAMES freetype/freetype.h HINTS ${FREETYPE_INC_SEARCH_PATH} ${FREETYPE_PKGC_INCLUDE_DIRS} PATH_SUFFIXES freetype2)
find_path(FREETYPE_FT2BUILD_INCLUDE_DIR NAMES ft2build.h HINTS ${FREETYPE_INC_SEARCH_PATH} ${FREETYPE_PKGC_INCLUDE_DIRS})

if (SYMBIAN) 
set(ORIGINAL_CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH})
set(CMAKE_PREFIX_PATH ${CMAKE_SYSYEM_OUT_DIR})
message(STATUS "Lib will be searched in Symbian out dir: ${CMAKE_SYSYEM_OUT_DIR}")
endif (SYMBIAN)
find_library(FREETYPE_LIBRARY_REL NAMES ${FREETYPE_LIBRARY_NAMES} HINTS ${FREETYPE_LIB_SEARCH_PATH} ${FREETYPE_PKGC_LIBRARY_DIRS} PATH_SUFFIXES "" release relwithdebinfo minsizerel)
find_library(FREETYPE_LIBRARY_DBG NAMES ${FREETYPE_LIBRARY_NAMES_DBG} HINTS ${FREETYPE_LIB_SEARCH_PATH} ${FREETYPE_PKGC_LIBRARY_DIRS} PATH_SUFFIXES "" debug)
if (SYMBIAN) 
set(CMAKE_PREFIX_PATH ${ORIGINAL_CMAKE_PREFIX_PATH})
endif (SYMBIAN)

make_library_set(FREETYPE_LIBRARY)

findpkg_finish(FREETYPE)
mark_as_advanced(FREETYPE_FT2BUILD_INCLUDE_DIR)
if (NOT FREETYPE_FT2BUILD_INCLUDE_DIR STREQUAL FREETYPE_INCLUDE_DIR)
  set(FREETYPE_INCLUDE_DIRS ${FREETYPE_INCLUDE_DIRS} ${FREETYPE_FT2BUILD_INCLUDE_DIR})
endif ()

# Reset framework finding
set(CMAKE_FIND_FRAMEWORK "FIRST")
