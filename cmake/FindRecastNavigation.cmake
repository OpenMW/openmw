# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
FindRecastNavigation
-------

Find the RecastNavigation include directory and library.

Use this module by invoking find_package with the form::

.. code-block:: cmake

  find_package(RecastNavigation
    [version]              # Minimum version e.g. 1.8.0
    [REQUIRED]             # Fail with error if RECASTNAV is not found
  )

Imported targets
^^^^^^^^^^^^^^^^

This module defines the following :prop_tgt:`IMPORTED` targets:

.. variable:: RecastNavigation::Recast

  Imported target for using the RECASTNAV library, if found.

Result variables
^^^^^^^^^^^^^^^^

.. variable:: RECASTNAV_FOUND

  Set to true if RECASTNAV library found, otherwise false or undefined.

.. variable:: RECASTNAV_INCLUDE_DIRS

  Paths to include directories listed in one variable for use by RECASTNAV client.

.. variable:: RECASTNAV_LIBRARIES

  Paths to libraries to linked against to use RECASTNAV.

.. variable:: RECAST_VERSION

  The version string of RECASTNAV found.

Cache variables
^^^^^^^^^^^^^^^

For users who wish to edit and control the module behavior, this module
reads hints about search locations from the following variables::

.. variable:: RECASTNAV_INCLUDE_DIR

  Path to RECASTNAV include directory with ``Recast.h`` header.

.. variable:: RECASTNAV_LIBRARY

  Path to RECASTNAV library to be linked.

NOTE: The variables above should not usually be used in CMakeLists.txt files!

#]=======================================================================]

### Find library ##############################################################

if(NOT RECASTNAV_LIBRARY)
    find_library(RECASTNAV_LIBRARY_RELEASE NAMES Recast)
    find_library(RECASTNAV_LIBRARY_DEBUG NAMES Recastd)

# TODO: figure out a way to get Recast, Detour and DebugUtils libs together
#    find_library(RECAST_LIBRARY_RELEASE NAMES Recast)
#    find_library(RECAST_LIBRARY_DEBUG NAMES Recastd)

#    find_library(DETOUR_LIBRARY_RELEASE NAMES Detour)
#    find_library(DETOUR_LIBRARY_DEBUG NAMES Detourd)

#    SET(RECASTNAV_LIBRARY_RELEASE ${RECAST_LIBRARY_RELEASE} ${DETOUR_LIBRARY_RELEASE})
#    SET(RECASTNAV_LIBRARY_DEBUG ${RECAST_LIBRARY_DEBUG} ${DETOUR_LIBRARY_DEBUG})

    include(SelectLibraryConfigurations)
    select_library_configurations(RECASTNAV)
else()
    file(TO_CMAKE_PATH "${RECASTNAV_LIBRARY}" RECASTNAV_LIBRARY)
endif()

### Find include directory ####################################################
find_path(RECASTNAV_INCLUDE_DIR NAMES Recast.h PATH_SUFFIXES include RECASTNAV include/recastnavigation)

if(RECASTNAV_INCLUDE_DIR AND EXISTS "${RECASTNAV_INCLUDE_DIR}/Recast.h")
    file(STRINGS "${RECASTNAV_INCLUDE_DIR}/Recast.h" _Recast_h_contents
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
        RECASTNAV_LIBRARY RECASTNAV_INCLUDE_DIR RECAST_VERSION)

mark_as_advanced(RECASTNAV_INCLUDE_DIR RECASTNAV_LIBRARY)

set(RECASTNAV_LIBRARIES ${RECASTNAV_LIBRARY})
set(RECASTNAV_INCLUDE_DIRS ${RECASTNAV_INCLUDE_DIR})

### Import targets ############################################################
if(RECASTNAV_FOUND)
    if(NOT TARGET RecastNavigation::Recast)
        add_library(RecastNavigation::Recast UNKNOWN IMPORTED)
        set_target_properties(RecastNavigation::Recast PROPERTIES
                IMPORTED_LINK_INTERFACE_LANGUAGES "C"
                INTERFACE_INCLUDE_DIRECTORIES "${RECASTNAV_INCLUDE_DIR}")

        if(RECASTNAV_LIBRARY_RELEASE)
            set_property(TARGET RecastNavigation::Recast APPEND PROPERTY
                    IMPORTED_CONFIGURATIONS RELEASE)
            set_target_properties(RecastNavigation::Recast PROPERTIES
                    IMPORTED_LOCATION_RELEASE "${RECASTNAV_LIBRARY_RELEASE}")
        endif()

        if(RECASTNAV_LIBRARY_DEBUG)
            set_property(TARGET RecastNavigation::Recast APPEND PROPERTY
                    IMPORTED_CONFIGURATIONS DEBUG)
            set_target_properties(RecastNavigation::Recast PROPERTIES
                    IMPORTED_LOCATION_DEBUG "${RECASTNAV_LIBRARY_DEBUG}")
        endif()

        if(NOT RECASTNAV_LIBRARY_RELEASE AND NOT RECASTNAV_LIBRARY_DEBUG)
            set_property(TARGET RecastNavigation::Recast APPEND PROPERTY
                    IMPORTED_LOCATION "${RECASTNAV_LIBRARY}")
        endif()
    endif()
endif()
