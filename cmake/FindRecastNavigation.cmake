# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.
# Copyright 2021 Bret Curtis for OpenMW
#[=======================================================================[.rst:
FindRecastNavigation
-------

Find the RecastNavigation include directory and library.

Use this module by invoking find_package with the form::

.. code-block:: cmake

  find_package(RecastNavigation
    [version]              # Minimum version e.g. 1.8.0
    [REQUIRED]             # Fail with error if RECAST is not found
  )

Imported targets
^^^^^^^^^^^^^^^^

This module defines the following :prop_tgt:`IMPORTED` targets:

.. variable:: RecastNavigation::Recast

  Imported target for using the RECAST library, if found.

Result variables
^^^^^^^^^^^^^^^^

.. variable:: RECAST_FOUND

  Set to true if RECAST library found, otherwise false or undefined.

.. variable:: RECAST_INCLUDE_DIRS

  Paths to include directories listed in one variable for use by RECAST client.

.. variable:: RECAST_LIBRARIES

  Paths to libraries to linked against to use RECAST.

.. variable:: RECAST_VERSION

  The version string of RECAST found.

Cache variables
^^^^^^^^^^^^^^^

For users who wish to edit and control the module behavior, this module
reads hints about search locations from the following variables::

.. variable:: RECAST_INCLUDE_DIR

  Path to RECAST include directory with ``Recast.h`` header.

.. variable:: RECAST_LIBRARY

  Path to RECAST library to be linked.

NOTE: The variables above should not usually be used in CMakeLists.txt files!

#]=======================================================================]

### Find libraries ##############################################################

if(NOT RECAST_LIBRARY)
    find_library(RECAST_LIBRARY_RELEASE NAMES Recast HINTS ${RECASTNAVIGATION_ROOT} PATH_SUFFIXES lib)
    find_library(RECAST_LIBRARY_DEBUG NAMES Recast-d HINTS ${RECASTNAVIGATION_ROOT} PATH_SUFFIXES lib)
    include(SelectLibraryConfigurations)
    select_library_configurations(RECAST)
    mark_as_advanced(RECAST_LIBRARY_RELEASE RECAST_LIBRARY_DEBUG)
else()
    file(TO_CMAKE_PATH "${RECAST_LIBRARY}" RECAST_LIBRARY)
endif()

if(NOT DETOUR_LIBRARY)
    find_library(DETOUR_LIBRARY_RELEASE NAMES Detour HINTS ${RECASTNAVIGATION_ROOT} PATH_SUFFIXES lib)
    find_library(DETOUR_LIBRARY_DEBUG NAMES Detour-d HINTS ${RECASTNAVIGATION_ROOT} PATH_SUFFIXES lib)
    include(SelectLibraryConfigurations)
    select_library_configurations(DETOUR)
    mark_as_advanced(DETOUR_LIBRARY_RELEASE DETOUR_LIBRARY_DEBUG)
else()
    file(TO_CMAKE_PATH "${DETOUR_LIBRARY}" DETOUR_LIBRARY)
endif()

if(NOT DEBUGUTILS_LIBRARY)
    find_library(DEBUGUTILS_LIBRARY_RELEASE NAMES DebugUtils HINTS ${RECASTNAVIGATION_ROOT} PATH_SUFFIXES lib)
    find_library(DEBUGUTILS_LIBRARY_DEBUG NAMES DebugUtils-d HINTS ${RECASTNAVIGATION_ROOT} PATH_SUFFIXES lib)
    include(SelectLibraryConfigurations)
    select_library_configurations(DEBUGUTILS)
    mark_as_advanced(DEBUGUTILS_LIBRARY_RELEASE DEBUGUTILS_LIBRARY_DEBUG)
else()
    file(TO_CMAKE_PATH "${DEBUGUTILS_LIBRARY}" DEBUGUTILS_LIBRARY)
endif()

### Find include directory ####################################################
find_path(RECAST_INCLUDE_DIR NAMES Recast.h HINTS ${RECASTNAVIGATION_ROOT} PATH_SUFFIXES include RECAST include/recastnavigation)
mark_as_advanced(RECAST_INCLUDE_DIR)

if(RECAST_INCLUDE_DIR AND EXISTS "${RECAST_INCLUDE_DIR}/Recast.h")
    file(STRINGS "${RECAST_INCLUDE_DIR}/Recast.h" _Recast_h_contents
            REGEX "#define RECAST_VERSION_[A-Z]+[ ]+[0-9]+")
    string(REGEX REPLACE "#define RECAST_VERSION_MAJOR[ ]+([0-9]+).+" "\\1"
            RECAST_VERSION_MAJOR "${_Recast_h_contents}")
    string(REGEX REPLACE ".+#define RECAST_VERSION_MINOR[ ]+([0-9]+).+" "\\1"
            RECAST_VERSION_MINOR "${_Recast_h_contents}")
    string(REGEX REPLACE ".+#define RECAST_VERSION_RELEASE[ ]+([0-9]+).*" "\\1"
            RECAST_VERSION_RELEASE "${_Recast_h_contents}")
    set(RECAST_VERSION "${RECAST_VERSION_MAJOR}.${RECAST_VERSION_MINOR}.${RECAST_VERSION_RELEASE}")
    unset(_Recast_h_contents)
endif()

#TODO: they don't include a version yet
set(RECAST_VERSION "1.5.1")

### Set result variables ######################################################
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(RecastNavigation DEFAULT_MSG
        RECAST_LIBRARY RECAST_INCLUDE_DIR RECAST_VERSION)

set(RECAST_LIBRARIES ${RECAST_LIBRARY})
set(RECAST_INCLUDE_DIRS ${RECAST_INCLUDE_DIR})

set(DETOUR_LIBRARIES ${DETOUR_LIBRARY})
set(DETOUR_INCLUDE_DIRS ${RECAST_INCLUDE_DIR})

set(DEBUGUTILS_LIBRARIES ${DEBUGUTILS_LIBRARY})
set(DEBUGUTILS_INCLUDE_DIRS ${RECAST_INCLUDE_DIR})

### Import targets ############################################################
if(RecastNavigation_FOUND)
    if(NOT TARGET RecastNavigation::Recast)
        add_library(RecastNavigation::Recast UNKNOWN IMPORTED)
        set_target_properties(RecastNavigation::Recast PROPERTIES
                IMPORTED_LINK_INTERFACE_LANGUAGES "C"
                INTERFACE_INCLUDE_DIRECTORIES "${RECAST_INCLUDE_DIR}")

        if(RECAST_LIBRARY_RELEASE)
            set_property(TARGET RecastNavigation::Recast APPEND PROPERTY
                    IMPORTED_CONFIGURATIONS RELEASE)
            set_target_properties(RecastNavigation::Recast PROPERTIES
                    IMPORTED_LOCATION_RELEASE "${RECAST_LIBRARY_RELEASE}")
        endif()

        if(RECAST_LIBRARY_DEBUG)
            set_property(TARGET RecastNavigation::Recast APPEND PROPERTY
                    IMPORTED_CONFIGURATIONS DEBUG)
            set_target_properties(RecastNavigation::Recast PROPERTIES
                    IMPORTED_LOCATION_DEBUG "${RECAST_LIBRARY_DEBUG}")
        endif()

        if(NOT RECAST_LIBRARY_RELEASE AND NOT RECAST_LIBRARY_DEBUG)
            set_property(TARGET RecastNavigation::Recast APPEND PROPERTY
                    IMPORTED_LOCATION "${RECAST_LIBRARY}")
        endif()
    endif()

    if(NOT TARGET RecastNavigation::Detour)
        add_library(RecastNavigation::Detour UNKNOWN IMPORTED)
        set_target_properties(RecastNavigation::Detour PROPERTIES
                IMPORTED_LINK_INTERFACE_LANGUAGES "C"
                INTERFACE_INCLUDE_DIRECTORIES "${DETOUR_INCLUDE_DIR}")

        if(DETOUR_LIBRARY_RELEASE)
            set_property(TARGET RecastNavigation::Detour APPEND PROPERTY
                    IMPORTED_CONFIGURATIONS RELEASE)
            set_target_properties(RecastNavigation::Detour PROPERTIES
                    IMPORTED_LOCATION_RELEASE "${DETOUR_LIBRARY_RELEASE}")
        endif()

        if(DETOUR_LIBRARY_DEBUG)
            set_property(TARGET RecastNavigation::Detour APPEND PROPERTY
                    IMPORTED_CONFIGURATIONS DEBUG)
            set_target_properties(RecastNavigation::Detour PROPERTIES
                    IMPORTED_LOCATION_DEBUG "${DETOUR_LIBRARY_DEBUG}")
        endif()

        if(NOT DETOUR_LIBRARY_RELEASE AND NOT DETOUR_LIBRARY_DEBUG)
            set_property(TARGET RecastNavigation::Detour APPEND PROPERTY
                    IMPORTED_LOCATION "${DETOUR_LIBRARY}")
        endif()
    endif()

    if(NOT TARGET RecastNavigation::DebugUtils)
        add_library(RecastNavigation::DebugUtils UNKNOWN IMPORTED)
        set_target_properties(RecastNavigation::DebugUtils PROPERTIES
                IMPORTED_LINK_INTERFACE_LANGUAGES "C"
                INTERFACE_INCLUDE_DIRECTORIES "${DEBUGUTILS_INCLUDE_DIR}")

        if(DEBUGUTILS_LIBRARY_RELEASE)
            set_property(TARGET RecastNavigation::DebugUtils APPEND PROPERTY
                    IMPORTED_CONFIGURATIONS RELEASE)
            set_target_properties(RecastNavigation::DebugUtils PROPERTIES
                    IMPORTED_LOCATION_RELEASE "${DEBUGUTILS_LIBRARY_RELEASE}")
        endif()

        if(DEBUGUTILS_LIBRARY_DEBUG)
            set_property(TARGET RecastNavigation::DebugUtils APPEND PROPERTY
                    IMPORTED_CONFIGURATIONS DEBUG)
            set_target_properties(RecastNavigation::DebugUtils PROPERTIES
                    IMPORTED_LOCATION_DEBUG "${DEBUGUTILS_LIBRARY_DEBUG}")
        endif()

        if(NOT DEBUGUTILS_LIBRARY_RELEASE AND NOT DEBUGUTILS_LIBRARY_DEBUG)
            set_property(TARGET RecastNavigation::DebugUtils APPEND PROPERTY
                    IMPORTED_LOCATION "${DEBUGUTILS_LIBRARY}")
        endif()
    endif()

endif()
