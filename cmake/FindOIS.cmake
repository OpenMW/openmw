#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

# - Try to find OIS
# Once done, this will define
#
#  OIS_FOUND - system has OIS
#  OIS_INCLUDE_DIRS - the OIS include directories 
#  OIS_LIBRARIES - link these to use OIS
#  OIS_BINARY_REL / OIS_BINARY_DBG - DLL names (windows only)

include(FindPkgMacros)
findpkg_begin(OIS)

# Get path, convert backslashes as ${ENV_${var}}
getenv_path(OIS_HOME)
getenv_path(OGRE_SDK)
getenv_path(OGRE_HOME)
getenv_path(OGRE_SOURCE)
getenv_path(OGRE_DEPENDENCIES_DIR)

# construct search paths
set(OIS_PREFIX_PATH ${OIS_HOME} ${ENV_OIS_HOME} 
  ${OGRE_DEPENDENCIES_DIR} ${ENV_OGRE_DEPENDENCIES_DIR}
  ${OGRE_SOURCE}/iPhoneDependencies ${ENV_OGRE_SOURCE}/iPhoneDependencies
  ${OGRE_SOURCE}/Dependencies ${ENV_OGRE_SOURCE}/Dependencies
  ${OGRE_SDK} ${ENV_OGRE_SDK}
  ${OGRE_HOME} ${ENV_OGRE_HOME})
create_search_paths(OIS)
# redo search if prefix path changed
clear_if_changed(OIS_PREFIX_PATH
  OIS_LIBRARY_FWK
  OIS_LIBRARY_REL
  OIS_LIBRARY_DBG
  OIS_INCLUDE_DIR
)

set(OIS_LIBRARY_NAMES OIS)
get_debug_names(OIS_LIBRARY_NAMES)

use_pkgconfig(OIS_PKGC OIS)

# For OIS, prefer static library over framework (important when referencing OIS source build)
set(CMAKE_FIND_FRAMEWORK "LAST")

findpkg_framework(OIS)
if (OIS_HOME)
  # OIS uses the 'includes' path for its headers in the source release, not 'include'
  set(OIS_INC_SEARCH_PATH ${OIS_INC_SEARCH_PATH} ${OIS_HOME}/includes)
endif()
if (APPLE AND OIS_HOME)
  # OIS source build on Mac stores libs in a different location
  # Also this is for static build
  set(OIS_LIB_SEARCH_PATH ${OIS_LIB_SEARCH_PATH} ${OIS_HOME}/Mac/XCode-2.2/build)
endif()
find_path(OIS_INCLUDE_DIR NAMES OIS.h HINTS ${OIS_INC_SEARCH_PATH} ${OIS_PKGC_INCLUDE_DIRS} PATH_SUFFIXES OIS)
find_library(OIS_LIBRARY_REL NAMES ${OIS_LIBRARY_NAMES} HINTS ${OIS_LIB_SEARCH_PATH} ${OIS_PKGC_LIBRARY_DIRS} PATH_SUFFIXES "" release relwithdebinfo minsizerel)
find_library(OIS_LIBRARY_DBG NAMES ${OIS_LIBRARY_NAMES_DBG} HINTS ${OIS_LIB_SEARCH_PATH} ${OIS_PKGC_LIBRARY_DIRS} PATH_SUFFIXES "" debug)
make_library_set(OIS_LIBRARY)

if (WIN32)
	set(OIS_BIN_SEARCH_PATH ${OIS_HOME}/dll ${ENV_OIS_HOME}/dll
		${OGRE_DEPENDENCIES_DIR}/bin ${ENV_OGRE_DEPENDENCIES_DIR}/bin
		${OGRE_SOURCE}/Dependencies/bin ${ENV_OGRE_SOURCE}/Dependencies/bin
		${OGRE_SDK}/bin ${ENV_OGRE_SDK}/bin
		${OGRE_HOME}/bin ${ENV_OGRE_HOME}/bin)
	find_file(OIS_BINARY_REL NAMES "OIS.dll" HINTS ${OIS_BIN_SEARCH_PATH}
	  PATH_SUFFIXES "" release relwithdebinfo minsizerel)
	find_file(OIS_BINARY_DBG NAMES "OIS_d.dll" HINTS ${OIS_BIN_SEARCH_PATH}
	  PATH_SUFFIXES "" debug )
endif()
mark_as_advanced(OIS_BINARY_REL OIS_BINARY_DBG)


findpkg_finish(OIS)

# add parent of OIS folder to support OIS/OIS.h
add_parent_dir(OIS_INCLUDE_DIRS OIS_INCLUDE_DIR)

# Reset framework finding
set(CMAKE_FIND_FRAMEWORK "FIRST")
