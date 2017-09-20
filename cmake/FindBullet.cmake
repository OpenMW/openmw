# - Try to find the Bullet physics engine
#
# This module accepts the following env variables
#  BULLET_ROOT - Can be set to bullet install path or Windows build path
#
# Once done this will define
#  Bullet_FOUND         - System has the all required components.
#  Bullet_INCLUDE_DIRS  - Include directory necessary for using the required components headers.
#  Bullet_LIBRARIES     - Link these to use the required bullet components.
#  Bullet_VERSION       - Version of libbullet
#
# For each of the components
#   - LinearMath
#   - BulletCollision
#   - BulletSoftBody
#   - BulletDynamics
#
# Copyright (c) 2009, Philip Lowman <philip at yhbt.com>
# Modified for OpenMW to parse BT_BULLET_VERSION.
#
# Redistribution AND use is allowed according to the terms of the New
# BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

include(LibFindMacros)

# Macro: _internal_find_bullet_library
# Checks for the given component by invoking pkgconfig etc.
macro(_internal_find_bullet_library _lib)
    libfind_pkg_detect(Bullet_${_lib} bullet
        FIND_LIBRARY ${_lib}
            HINTS $ENV{BULLET_ROOT}
            PATH_SUFFIXES lib
    )
    libfind_process(Bullet_${_lib})
endmacro()

set(_known_components LinearMath BulletCollision BulletSoftBody BulletDynamics)

# Check if the required components were found and add their stuff to the Bullet_* vars.
foreach (_component ${Bullet_FIND_COMPONENTS})
    list(FIND _known_components ${_component} _known_component)
    if (_known_component EQUAL -1)
        message(FATAL_ERROR "Unknown component '${_component}'")
    endif()

    set(Bullet_${_component}_Debug_FIND_QUIETLY TRUE) # don't spam messages with optional Debug component
    _internal_find_bullet_library(${_component})
    _internal_find_bullet_library(${_component}_Debug)

    if (Bullet_${_component}_Debug_FOUND)
        set(Bullet_LIBRARIES ${Bullet_LIBRARIES} optimized ${Bullet_${_component}_LIBRARIES} debug ${Bullet_${_component}_Debug_LIBRARIES})
    else()
        set(Bullet_LIBRARIES ${Bullet_LIBRARIES} ${Bullet_${_component}_LIBRARIES})
    endif()
endforeach()

libfind_pkg_detect(Bullet bullet
    FIND_PATH btBulletCollisionCommon.h
        HINTS $ENV{BULLET_ROOT}
        PATH_SUFFIXES include/bullet
)
set(Bullet_INCLUDE_DIRS ${Bullet_INCLUDE_DIR})
libfind_version_header(Bullet LinearMath/btScalar.h BT_BULLET_VERSION)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(Bullet
    FOUND_VAR Bullet_FOUND
    VERSION_VAR Bullet_VERSION
    HANDLE_COMPONENTS
    REQUIRED_VARS
        Bullet_LIBRARIES
        Bullet_INCLUDE_DIR
)
