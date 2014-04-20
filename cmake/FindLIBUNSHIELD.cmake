# Locate LIBUNSHIELD
# This module defines
# LIBUNSHIELD_LIBRARY
# LIBUNSHIELD_FOUND, if false, do not try to link to LibUnshield
# LIBUNSHIELD_INCLUDE_DIR, where to find the headers
#
# Created by Tom Mason (wheybags) for OpenMW (http://openmw.org), based on FindMPG123.cmake
#
# Ripped off from other sources. In fact, this file is so generic (I
# just did a search and replace on another file) that I wonder why the
# CMake guys haven't wrapped this entire thing in a single
# function. Do we really need to repeat this stuff for every single
# library when they all work the same? </today's rant>

FIND_PATH(LIBUNSHIELD_INCLUDE_DIR libunshield.h
  HINTS
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
  /sw # Fink
  /opt/local # DarwinPorts
  /opt/csw # Blastwave
  /opt
  /usr/include
)

FIND_LIBRARY(LIBUNSHIELD_LIBRARY 
  unshield
  HINTS
#  PATH_SUFFIXES lib64 lib libs64 libs libs/Win32 libs/Win64
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
  /sw
  /opt/local
  /opt/csw
  /opt
  /usr/lib
)

IF(LIBUNSHIELD_LIBRARY AND LIBUNSHIELD_INCLUDE_DIR)
  SET(LIBUNSHIELD_FOUND "YES")
ENDIF(LIBUNSHIELD_LIBRARY AND LIBUNSHIELD_INCLUDE_DIR)

