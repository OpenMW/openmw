# Locate SNDFILE
# This module defines
# SNDFILE_LIBRARY
# SNDFILE_FOUND, if false, do not try to link to Sndfile 
# SNDFILE_INCLUDE_DIR, where to find the headers
#
# Created by Nicolay Korslund for OpenMW (http://openmw.com)

FIND_PATH(SNDFILE_INCLUDE_DIR sndfile.h
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

FIND_LIBRARY(SNDFILE_LIBRARY 
  NAMES sndfile
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

SET(SNDFILE_FOUND "NO")
IF(SNDFILE_LIBRARY AND SNDFILE_INCLUDE_DIR)
  SET(SNDFILE_FOUND "YES")
ENDIF(SNDFILE_LIBRARY AND SNDFILE_INCLUDE_DIR)

