# - Find MyGUI includes and library
#
# This module accepts the following env variables
#  MYGUI_HOME - Can be set to MyGUI install path or Windows build path
#
# This module defines
# MyGUI_INCLUDE_DIRS
# MyGUI_LIBRARIES, the libraries to link against to use MyGUI.
# MyGUI_FOUND, If false, do not try to use MyGUI
#
# Copyright © 2007, Matt Williams
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

include(LibFindMacros)

if (MYGUI_STATIC)
    set(MYGUI_STATIC_SUFFIX "Static")
else()
    set(MYGUI_STATIC_SUFFIX "")
endif()

libfind_pkg_detect(MyGUI_Debug MyGUI${MYGUI_STATIC_SUFFIX} MYGUI${MYGUI_STATIC_SUFFIX}
    FIND_LIBRARY MyGUIEngine_d${MYGUI_STATIC_SUFFIX}
        HINTS $ENV{MYGUI_HOME}/lib
        PATH_SUFFIXES "" debug
)
set(MyGUI_Debug_FIND_QUIETLY TRUE)
libfind_process(MyGUI_Debug)

libfind_pkg_detect(MyGUI MyGUI${MYGUI_STATIC_SUFFIX} MYGUI${MYGUI_STATIC_SUFFIX}
    FIND_PATH MyGUI.h
        HINTS $ENV{MYGUI_HOME}/include
        PATH_SUFFIXES MYGUI MyGUI
    FIND_LIBRARY MyGUIEngine${MYGUI_STATIC_SUFFIX}
        HINTS $ENV{MYGUI_HOME}/lib
        PATH_SUFFIXES "" release relwithdebinfo minsizerel
)
if (MYGUI_STATIC AND (APPLE OR ANDROID))
    # we need explicit Freetype libs only on OS X and ANDROID for static build
    libfind_package(MyGUI Freetype)
endif()

libfind_version_n_header(MyGUI
    NAMES MyGUI_Prerequest.h
    DEFINES MYGUI_VERSION_MAJOR MYGUI_VERSION_MINOR MYGUI_VERSION_PATCH
)
libfind_process(MyGUI)

if (MyGUI_Debug_FOUND)
    set(MyGUI_LIBRARIES optimized ${MyGUI_LIBRARIES} debug ${MyGUI_Debug_LIBRARIES})
endif()
