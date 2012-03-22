#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

# - Try to find zziplib
# Once done, this will define
#
#  ZZip_FOUND - system has ZZip
#  ZZip_INCLUDE_DIRS - the ZZip include directories 
#  ZZip_LIBRARIES - link these to use ZZip

include(FindPkgMacros)
findpkg_begin(ZZip)

# Get path, convert backslashes as ${ENV_${var}}
getenv_path(ZZIP_HOME)


# construct search paths
set(ZZip_PREFIX_PATH ${ZZIP_HOME} ${ENV_ZZIP_HOME})
create_search_paths(ZZip)
# redo search if prefix path changed
clear_if_changed(ZZip_PREFIX_PATH
  ZZip_LIBRARY_FWK
  ZZip_LIBRARY_REL
  ZZip_LIBRARY_DBG
  ZZip_INCLUDE_DIR
)

set(ZZip_LIBRARY_NAMES zzip zziplib)
get_debug_names(ZZip_LIBRARY_NAMES)

use_pkgconfig(ZZip_PKGC zziplib)

findpkg_framework(ZZip)

find_path(ZZip_INCLUDE_DIR NAMES zzip/zzip.h HINTS ${ZZip_INC_SEARCH_PATH} ${ZZip_PKGC_INCLUDE_DIRS})
find_library(ZZip_LIBRARY_REL NAMES ${ZZip_LIBRARY_NAMES} HINTS ${ZZip_LIB_SEARCH_PATH} ${ZZip_PKGC_LIBRARY_DIRS} PATH_SUFFIXES "" release relwithdebinfo minsizerel)
find_library(ZZip_LIBRARY_DBG NAMES ${ZZip_LIBRARY_NAMES_DBG} HINTS ${ZZip_LIB_SEARCH_PATH} ${ZZip_PKGC_LIBRARY_DIRS} PATH_SUFFIXES "" debug)
make_library_set(ZZip_LIBRARY)

findpkg_finish(ZZip)

