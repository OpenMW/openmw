# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
FindLZ4
-------

Find the LZ4 include directory and library.

Use this module by invoking find_package with the form::

.. code-block:: cmake

  find_package(LZ4
    [version]              # Minimum version e.g. 1.8.0
    [REQUIRED]             # Fail with error if LZ4 is not found
  )

Imported targets
^^^^^^^^^^^^^^^^

This module defines the following :prop_tgt:`IMPORTED` targets:

.. variable:: LZ4::LZ4

  Imported target for using the LZ4 library, if found.

Result variables
^^^^^^^^^^^^^^^^

.. variable:: LZ4_FOUND

  Set to true if LZ4 library found, otherwise false or undefined.

.. variable:: LZ4_INCLUDE_DIRS

  Paths to include directories listed in one variable for use by LZ4 client.

.. variable:: LZ4_LIBRARIES

  Paths to libraries to linked against to use LZ4.

.. variable:: LZ4_VERSION

  The version string of LZ4 found.

Cache variables
^^^^^^^^^^^^^^^

For users who wish to edit and control the module behavior, this module
reads hints about search locations from the following variables::

.. variable:: LZ4_INCLUDE_DIR

  Path to LZ4 include directory with ``lz4.h`` header.

.. variable:: LZ4_LIBRARY

  Path to LZ4 library to be linked.

NOTE: The variables above should not usually be used in CMakeLists.txt files!

#]=======================================================================]

### Find library ##############################################################

if(NOT LZ4_LIBRARY)
    find_library(LZ4_LIBRARY_RELEASE NAMES lz4)
    find_library(LZ4_LIBRARY_DEBUG NAMES lz4d)

    include(SelectLibraryConfigurations)
    select_library_configurations(LZ4)
else()
    file(TO_CMAKE_PATH "${LZ4_LIBRARY}" LZ4_LIBRARY)
endif()

### Find include directory ####################################################
find_path(LZ4_INCLUDE_DIR NAMES lz4.h)

if(LZ4_INCLUDE_DIR AND EXISTS "${LZ4_INCLUDE_DIR}/lz4.h")
    file(STRINGS "${LZ4_INCLUDE_DIR}/lz4.h" _lz4_h_contents
            REGEX "#define LZ4_VERSION_[A-Z]+[ ]+[0-9]+")
    string(REGEX REPLACE "#define LZ4_VERSION_MAJOR[ ]+([0-9]+).+" "\\1"
            LZ4_VERSION_MAJOR "${_lz4_h_contents}")
    string(REGEX REPLACE ".+#define LZ4_VERSION_MINOR[ ]+([0-9]+).+" "\\1"
            LZ4_VERSION_MINOR "${_lz4_h_contents}")
    string(REGEX REPLACE ".+#define LZ4_VERSION_RELEASE[ ]+([0-9]+).*" "\\1"
            LZ4_VERSION_RELEASE "${_lz4_h_contents}")
    set(LZ4_VERSION "${LZ4_VERSION_MAJOR}.${LZ4_VERSION_MINOR}.${LZ4_VERSION_RELEASE}")
    unset(_lz4_h_contents)
endif()

### Set result variables ######################################################
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LZ4 DEFAULT_MSG
        LZ4_LIBRARY LZ4_INCLUDE_DIR LZ4_VERSION)

mark_as_advanced(LZ4_INCLUDE_DIR LZ4_LIBRARY)

set(LZ4_LIBRARIES ${LZ4_LIBRARY})
set(LZ4_INCLUDE_DIRS ${LZ4_INCLUDE_DIR})

### Import targets ############################################################
if(LZ4_FOUND)
    if(NOT TARGET LZ4::LZ4)
        add_library(LZ4::LZ4 UNKNOWN IMPORTED)
        set_target_properties(LZ4::LZ4 PROPERTIES
                IMPORTED_LINK_INTERFACE_LANGUAGES "C"
                INTERFACE_INCLUDE_DIRECTORIES "${LZ4_INCLUDE_DIR}")

        if(LZ4_LIBRARY_RELEASE)
            set_property(TARGET LZ4::LZ4 APPEND PROPERTY
                    IMPORTED_CONFIGURATIONS RELEASE)
            set_target_properties(LZ4::LZ4 PROPERTIES
                    IMPORTED_LOCATION_RELEASE "${LZ4_LIBRARY_RELEASE}")
        endif()

        if(LZ4_LIBRARY_DEBUG)
            set_property(TARGET LZ4::LZ4 APPEND PROPERTY
                    IMPORTED_CONFIGURATIONS DEBUG)
            set_target_properties(LZ4::LZ4 PROPERTIES
                    IMPORTED_LOCATION_DEBUG "${LZ4_LIBRARY_DEBUG}")
        endif()

        if(NOT LZ4_LIBRARY_RELEASE AND NOT LZ4_LIBRARY_DEBUG)
            set_property(TARGET LZ4::LZ4 APPEND PROPERTY
                    IMPORTED_LOCATION "${LZ4_LIBRARY}")
        endif()
    endif()
endif()
