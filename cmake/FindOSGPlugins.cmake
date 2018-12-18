# This module accepts the following env variable
#  OSGPlugins_LIB_DIR - <OpenSceneGraph>/lib/osgPlugins-<X.X.X> , path to search plugins
#  OSGPlugins_DONT_FIND_DEPENDENCIES - Set to skip also finding png, zlib and jpeg
#
# Once done this will define
#  OSGPlugins_FOUND         - System has the all required components.
#  OSGPlugins_LIBRARIES     - Link these to use the required osg plugins components.
#
# Components:
#   - osgdb_png
#   - osgdb_tga
#   - osgdb_dds
#   - osgdb_jpeg

include(LibFindMacros)
include(Findosg_functions)

if (NOT OSGPlugins_LIB_DIR)
    set(_mode WARNING)
    if (OSGPlugins_FIND_REQUIRED)
        set(_mode FATAL_ERROR)
    endif()
    message(${_mode} "OSGPlugins_LIB_DIR variable must be set")
endif()

foreach(_library ${OSGPlugins_FIND_COMPONENTS})
    string(TOUPPER ${_library} _library_uc)
    set(_component OSGPlugins_${_library})

    set(${_library_uc}_DIR ${OSGPlugins_LIB_DIR}) # to help function osg_find_library
    set(_saved_lib_prefix ${CMAKE_FIND_LIBRARY_PREFIXES}) # save CMAKE_FIND_LIBRARY_PREFIXES
    set(CMAKE_FIND_LIBRARY_PREFIXES "") # search libraries with no prefix
    osg_find_library(${_library_uc} ${_library}) # find it into ${_library_uc}_LIBRARIES
    set(CMAKE_FIND_LIBRARY_PREFIXES ${_saved_lib_prefix}) # restore prefix

    if (${_library_uc}_LIBRARIES)
        set(${_component}_LIBRARY ${${_library_uc}_LIBRARIES}) # fake as if we call find_library
    else()
        set(${_component}_LIBRARY ${_component}_LIBRARY-NOTFOUND)
    endif()

    list(APPEND OSGPlugins_PROCESS_LIBS ${_component}_LIBRARY)
endforeach()

if(NOT DEFINED OSGPlugins_DONT_FIND_DEPENDENCIES)
    foreach(_dependency PNG ZLIB JPEG) # needed by osgdb_png or osgdb_jpeg
        libfind_package(OSGPlugins ${_dependency})
        set(${_dependency}_LIBRARY_OPTS ${_dependency}_LIBRARY)
        #list(APPEND OSGPlugins_PROCESS_LIBS ${_dependency}_LIBRARY)
    endforeach()
endif()

libfind_process(OSGPlugins)
