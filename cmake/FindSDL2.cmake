# Locate SDL2 library
# This module defines
# SDL2_LIBRARY, the SDL2 library, with no other libraries
# SDL2_LIBRARIES, the SDL library and required components with compiler flags
# SDL2_FOUND, if false, do not try to link to SDL2
# SDL2_INCLUDE_DIR, where to find SDL.h
# SDL2_VERSION, the version of the found library
#
# This module accepts the following env variables
#  SDL2DIR - Can be set to ./configure --prefix=$SDL2DIR used in building SDL2. l.e.galup 9-20-02
# This module responds to the the flag:
# SDL2_BUILDING_LIBRARY
# If this is defined, then no SDL2_main will be linked in because
# only applications need main().
# Otherwise, it is assumed you are building an application and this
# module will attempt to locate and set the the proper link flags
# as part of the returned SDL2_LIBRARIES variable.
#
# Don't forget to include SDL2main.h and SDL2main.m your project for the
# OS X framework based version. (Other versions link to -lSDL2main which
# this module will try to find on your behalf.) Also for OS X, this
# module will automatically add the -framework Cocoa on your behalf.
#
#
# Modified by Eric Wing.
# Added code to assist with automated building by using environmental variables
# and providing a more controlled/consistent search behavior.
# Added new modifications to recognize OS X frameworks and
# additional Unix paths (FreeBSD, etc).
# Also corrected the header search path to follow "proper" SDL2 guidelines.
# Added a search for SDL2main which is needed by some platforms.
# Added a search for threads which is needed by some platforms.
# Added needed compile switches for MinGW.
#
# On OSX, this will prefer the Framework version (if found) over others.
# People will have to manually change the cache values of
# SDL2_LIBRARY to override this selection or set the CMake environment
# CMAKE_INCLUDE_PATH to modify the search paths.
#
# Note that the header path has changed from SDL2/SDL.h to just SDL.h
# This needed to change because "proper" SDL2 convention
# is #include "SDL.h", not <SDL2/SDL.h>. This is done for portability
# reasons because not all systems place things in SDL2/ (see FreeBSD).
#
# Ported by Johnny Patterson. This is a literal port for SDL2 of the FindSDL.cmake
# module with the minor edit of changing "SDL" to "SDL2" where necessary. This
# was not created for redistribution, and exists temporarily pending official
# SDL2 CMake modules.

#=============================================================================
# Copyright 2003-2009 Kitware, Inc.
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)


if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(_sdl_lib_suffix lib/x64)
else()
    set(_sdl_lib_suffix lib/x86)
endif()

libfind_pkg_detect(SDL2 sdl2
    FIND_PATH SDL.h
        HINTS $ENV{SDL2DIR}
        PATH_SUFFIXES include SDL2 include/SDL2
    FIND_LIBRARY SDL2
        HINTS $ENV{SDL2DIR}
        PATH_SUFFIXES lib ${_sdl_lib_suffix}
)
libfind_version_n_header(SDL2 NAMES SDL_version.h DEFINES SDL_MAJOR_VERSION SDL_MINOR_VERSION SDL_PATCHLEVEL)

IF(NOT SDL2_BUILDING_LIBRARY AND NOT APPLE)
    # Non-OS X framework versions expect you to also dynamically link to
    # SDL2main. This is mainly for Windows and OS X. Other (Unix) platforms
    # seem to provide SDL2main for compatibility even though they don't
    # necessarily need it.
    libfind_pkg_detect(SDL2MAIN sdl2
        FIND_LIBRARY SDL2main
            HINTS $ENV{SDL2DIR}
            PATH_SUFFIXES lib ${_sdl_lib_suffix}
    )
    set(SDL2MAIN_FIND_QUIETLY TRUE)
    libfind_process(SDL2MAIN)
    list(APPEND SDL2_PROCESS_LIBS SDL2MAIN_LIBRARY)
ENDIF()


set(SDL2_TARGET_SPECIFIC)

if (APPLE)
    # For OS X, SDL2 uses Cocoa as a backend so it must link to Cocoa.
    list(APPEND SDL2_TARGET_SPECIFIC "-framework Cocoa")
else()
    # SDL2 may require threads on your system.
    # The Apple build may not need an explicit flag because one of the
    # frameworks may already provide it.
    # But for non-OSX systems, I will use the CMake Threads package.
    libfind_package(SDL2 Threads)
    list(APPEND SDL2_TARGET_SPECIFIC ${CMAKE_THREAD_LIBS_INIT})
endif()

# MinGW needs an additional library, mwindows
# It's total link flags should look like -lmingw32 -lSDL2main -lSDL2 -lmwindows
# (Actually on second look, I think it only needs one of the m* libraries.)
if(MINGW)
    list(APPEND SDL2_TARGET_SPECIFIC mingw32)
endif()

if(WIN32)
    list(APPEND SDL2_TARGET_SPECIFIC winmm imm32 version msimg32)
endif()

set(SDL2_PROCESS_LIBS SDL2_TARGET_SPECIFIC)

libfind_process(SDL2)

if (SDL2_STATIC AND UNIX AND NOT APPLE)
    execute_process(COMMAND sdl2-config --static-libs OUTPUT_VARIABLE SDL2_STATIC_FLAGS)
    string(REGEX REPLACE "(\r?\n)+$" "" SDL2_STATIC_FLAGS "${SDL2_STATIC_FLAGS}")
    set(SDL2_LIBRARIES ${SDL2_STATIC_FLAGS})
endif()
