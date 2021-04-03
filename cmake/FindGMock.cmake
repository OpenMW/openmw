# Get the Google C++ Mocking Framework.
# (This file is almost an copy of the original FindGTest.cmake file for GMock,
#  feel free to use it as it is or modify it for your own needs.)
#
# Defines the following variables:
#
#   GMOCK_FOUND - Found or got the Google Mocking framework
#   GMOCK_INCLUDE_DIRS - GMock include directory
#
# Also defines the library variables below as normal variables
#
#   GMOCK_BOTH_LIBRARIES - Both libgmock & libgmock_main
#   GMOCK_LIBRARIES - libgmock
#   GMOCK_MAIN_LIBRARIES - libgmock-main
#
# Accepts the following variables as input:
#
#   GMOCK_SRC_DIR -The directory of the gmock sources
#   GMOCK_VER - The version of the gmock sources to be downloaded
#
#-----------------------
# Example Usage:
#
#    set(GMOCK_ROOT "~/gmock")
#    find_package(GMock REQUIRED)
#    include_directories(${GMOCK_INCLUDE_DIRS})
#
#    add_executable(foo foo.cc)
#    target_link_libraries(foo ${GMOCK_BOTH_LIBRARIES})
#
#=============================================================================
# Copyright (c) 2016 Michel Estermann
# Copyright (c) 2016 Kamil Strzempowicz
# Copyright (c) 2011 Matej Svec
#
# CMake - Cross Platform Makefile Generator
# Copyright 2000-2016 Kitware, Inc.
# Copyright 2000-2011 Insight Software Consortium
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# * Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
#
# * Neither the names of Kitware, Inc., the Insight Software Consortium,
#   nor the names of their contributors may be used to endorse or promote
#   products derived from this software without specific prior written
#   permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# ------------------------------------------------------------------------------
#
# The above copyright and license notice applies to distributions of
# CMake in source and binary form.  Some source files contain additional
# notices of original copyright by their contributors; see each source
# for details.  Third-party software packages supplied with CMake under
# compatible licenses provide their own copyright notices documented in
# corresponding subdirectories.
#
# ------------------------------------------------------------------------------
#
# CMake was initially developed by Kitware with the following sponsorship:
#
#  * National Library of Medicine at the National Institutes of Health
#    as part of the Insight Segmentation and Registration Toolkit (ITK).
#
#  * US National Labs (Los Alamos, Livermore, Sandia) ASC Parallel
#    Visualization Initiative.
#
#  * National Alliance for Medical Image Computing (NAMIC) is funded by the
#    National Institutes of Health through the NIH Roadmap for Medical Research,
#    Grant U54 EB005149.
#
#  * Kitware, Inc.
#=============================================================================

function(_gmock_find_library _name)
    find_library(${_name}
        NAMES ${ARGN}
        HINTS
        ENV GMOCK_ROOT
        ${GMOCK_ROOT}
        PATH_SUFFIXES ${_gmock_libpath_suffixes}
        )
    mark_as_advanced(${_name})
endfunction()

if(NOT DEFINED GMOCK_MSVC_SEARCH)
    set(GMOCK_MSVC_SEARCH MD)
endif()

set(_gmock_libpath_suffixes lib)
if(MSVC)
    if(GMOCK_MSVC_SEARCH STREQUAL "MD")
        list(APPEND _gmock_libpath_suffixes
            msvc/gmock-md/Debug
            msvc/gmock-md/Release)
    elseif(GMOCK_MSVC_SEARCH STREQUAL "MT")
        list(APPEND _gmock_libpath_suffixes
            msvc/gmock/Debug
            msvc/gmock/Release)
    endif()
endif()

find_path(GMOCK_INCLUDE_DIR gmock/gmock.h
    HINTS
    $ENV{GMOCK_ROOT}/include
    ${GMOCK_ROOT}/include
    )
mark_as_advanced(GMOCK_INCLUDE_DIR)

if(MSVC AND GMOCK_MSVC_SEARCH STREQUAL "MD")
    # The provided /MD project files for Google Mock add -md suffixes to the
    # library names.
    _gmock_find_library(GMOCK_LIBRARY gmock-md gmock)
    _gmock_find_library(GMOCK_LIBRARY_DEBUG gmock-mdd gmockd)
    _gmock_find_library(GMOCK_MAIN_LIBRARY gmock_main-md gmock_main)
    _gmock_find_library(GMOCK_MAIN_LIBRARY_DEBUG gmock_main-mdd gmock_maind)
else()
    _gmock_find_library(GMOCK_LIBRARY gmock)
    _gmock_find_library(GMOCK_LIBRARY_DEBUG gmockd)
    _gmock_find_library(GMOCK_MAIN_LIBRARY gmock_main)
    _gmock_find_library(GMOCK_MAIN_LIBRARY_DEBUG gmock_maind)

endif()

if(NOT TARGET GMock::GMock)
    add_library(GMock::GMock UNKNOWN IMPORTED)
endif()

if(NOT TARGET GMock::Main)
    add_library(GMock::Main UNKNOWN IMPORTED)
endif()

set(GMOCK_LIBRARY_EXISTS OFF)

if(EXISTS "${GMOCK_LIBRARY}" OR EXISTS "${GMOCK_LIBRARY_DEBUG}" AND GMOCK_INCLUDE_DIR)
    set(GMOCK_LIBRARY_EXISTS ON)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GMock DEFAULT_MSG GMOCK_LIBRARY GMOCK_INCLUDE_DIR GMOCK_MAIN_LIBRARY)

include(CMakeFindDependencyMacro)
find_dependency(Threads)

set_target_properties(GMock::GMock PROPERTIES
    INTERFACE_LINK_LIBRARIES "Threads::Threads"
    IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
    IMPORTED_LOCATION "${GMOCK_LIBRARY}")

if(GMOCK_INCLUDE_DIR)
    set_target_properties(GMock::GMock PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${GMOCK_INCLUDE_DIR}"
        INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${GMOCK_INCLUDE_DIR}"
    )
    if(GMOCK_VER VERSION_LESS "1.7")
        # GMock 1.6 still has GTest as an external link-time dependency,
        # so just specify it on the link interface.
        set_property(TARGET GMock::GMock APPEND PROPERTY
            INTERFACE_LINK_LIBRARIES GTest::GTest)
    endif()
endif()

set_target_properties(GMock::Main PROPERTIES
    INTERFACE_LINK_LIBRARIES "GMock::GMock"
    IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
    IMPORTED_LOCATION "${GMOCK_MAIN_LIBRARY}")

if(GMOCK_FOUND)
    set(GMOCK_INCLUDE_DIRS ${GMOCK_INCLUDE_DIR})
    set(GMOCK_LIBRARIES GMock::GMock)
    set(GMOCK_MAIN_LIBRARIES GMock::Main)
    set(GMOCK_BOTH_LIBRARIES ${GMOCK_LIBRARIES} ${GMOCK_MAIN_LIBRARIES})
    if(VERBOSE)
        message(STATUS "GMock includes: ${GMOCK_INCLUDE_DIRS}")
        message(STATUS "GMock libs: ${GMOCK_BOTH_LIBRARIES}")
    endif()
endif()
