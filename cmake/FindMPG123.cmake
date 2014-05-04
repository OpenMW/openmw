# Locate MPG123
# This module defines
# MPG123_LIBRARY
# MPG123_FOUND, if false, do not try to link to Mpg123 
# MPG123_INCLUDE_DIR, where to find the headers
#
# Created by Nicolay Korslund for OpenMW (http://openmw.com)
#
# Ripped off from other sources. In fact, this file is so generic (I
# just did a search and replace on another file) that I wonder why the
# CMake guys haven't wrapped this entire thing in a single
# function. Do we really need to repeat this stuff for every single
# library when they all work the same? </today's rant>

FIND_PATH(MPG123_INCLUDE_DIR mpg123.h
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
)

FIND_LIBRARY(MPG123_LIBRARY 
  NAMES mpg123
  HINTS
  PATH_SUFFIXES lib64 lib libs64 libs libs/Win32 libs/Win64
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
  /sw
  /opt/local
  /opt/csw
  /opt
)

SET(MPG123_FOUND "NO")
IF(MPG123_LIBRARY AND MPG123_INCLUDE_DIR)
  SET(MPG123_FOUND "YES")
ENDIF(MPG123_LIBRARY AND MPG123_INCLUDE_DIR)

