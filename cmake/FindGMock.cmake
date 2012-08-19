# Locate the Google C++ Mocking Framework.
#
# Defines the following variables:
#
#   GMOCK_FOUND - Found the Google Mocking framework
#   GMOCK_INCLUDE_DIRS - Include directories
#
# Also defines the library variables below as normal
# variables.  These contain debug/optimized keywords when
# a debugging library is found.
#
#   GMOCK_BOTH_LIBRARIES - Both libgmock & libgmock-main
#   GMOCK_LIBRARIES - libgmock
#   GMOCK_MAIN_LIBRARIES - libgmock-main
#
# Accepts the following variables as input:
#
#   GMOCK_ROOT - (as CMake or env. variable)
#                The root directory of the gmock install prefix
#
#-----------------------
# Example Usage:
#
#    enable_testing(true)
#    find_package(GMock REQUIRED)
#    include_directories(${GMOCK_INCLUDE_DIRS})
#
#    add_executable(foo foo.cc)
#    target_link_libraries(foo ${GMOCK_BOTH_LIBRARIES})
#
#    add_test(AllTestsInFoo foo)
#

#set (GMOCK_FOUND FALSE)


#set (GMOCK_ROOT $ENV{GMOCK_ROOT} CACHE PATH "Path to the gmock root directory.")
if (NOT EXISTS ${GMOCK_ROOT})
    message (FATAL_ERROR "GMOCK_ROOT does not exist.")
endif ()

#set (GMOCK_BUILD ${GMOCK_ROOT}/build CACHE PATH "Path to the gmock build directory.")
if (NOT EXISTS ${GMOCK_BUILD})
    message (FATAL_ERROR "GMOCK_BUILD does not exist.")
endif ()

# Find the include directory
find_path(GMOCK_INCLUDE_DIRS gmock/gmock.h
    HINTS
        $ENV{GMOCK_ROOT}/include
        ${GMOCK_ROOT}/include
)
mark_as_advanced(GMOCK_INCLUDE_DIRS)

function(_gmock_find_library _name)
    find_library(${_name}
        NAMES ${ARGN}
        HINTS
            $ENV{GMOCK_BUILD}
            ${GMOCK_BUILD}
    )
    mark_as_advanced(${_name})
endfunction()

# Find the gmock libraries
if (MSVC)
    _gmock_find_library (GMOCK_LIBRARIES_DEBUG   gmock ${GMOCK_BUILD}/Debug)
    _gmock_find_library (GMOCK_LIBRARIES_RELEASE gmock ${GMOCK_BUILD}/Release)
    _gmock_find_library (GMOCK_MAIN_LIBRARIES_DEBUG   gmock_main ${GMOCK_BUILD}/Debug)
    _gmock_find_library (GMOCK_MAIN_LIBRARIES_RELEASE gmock_main ${GMOCK_BUILD}/Release)
    set (GMOCK_LIBRARIES
        debug ${GMOCK_LIBRARIES_DEBUG}
        optimized ${GMOCK_LIBRARIES_RELEASE}
    )
    set (GMOCK_MAIN_LIBRARIES
        debug ${GMOCK_MAIN_LIBRARIES_DEBUG}
        optimized ${GMOCK_MAIN_LIBRARIES_RELEASE}
    )
else ()
    _gmock_find_library (GMOCK_LIBRARIES      gmock      ${GMOCK_BUILD})
    _gmock_find_library (GMOCK_MAIN_LIBRARIES gmock_main ${GMOCK_BUILD} ${GMOCK_BUILD}/Debug)
endif ()

FIND_PACKAGE_HANDLE_STANDARD_ARGS(GMock DEFAULT_MSG GMOCK_LIBRARIES GMOCK_INCLUDE_DIRS GMOCK_MAIN_LIBRARIES)

if(GMOCK_FOUND)
    set(GMOCK_INCLUDE_DIRS ${GMOCK_INCLUDE_DIR})
    set(GMOCK_BOTH_LIBRARIES ${GMOCK_LIBRARIES} ${GMOCK_MAIN_LIBRARIES})
endif()


