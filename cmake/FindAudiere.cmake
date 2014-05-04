# Locate Audiere
# This module defines
# AUDIERE_LIBRARY
# AUDIERE_FOUND, if false, do not try to link to Audiere 
# AUDIERE_INCLUDE_DIR, where to find the headers
#
# Created by Nicolay Korslund for OpenMW (http://openmw.com)
#
# More or less a direct ripoff of FindOpenAL.cmake by Eric Wing.

#=============================================================================
# Copyright 2005-2009 Kitware, Inc.
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distributed this file outside of CMake, substitute the full
#  License text for the above reference.)


FIND_PATH(AUDIERE_INCLUDE_DIR audiere.h
  HINTS
  PATH_SUFFIXES include
  PATHS
  $ENV{AUDIERE_DIR}
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
  /sw # Fink
  /opt/local # DarwinPorts
  /opt/csw # Blastwave
  /opt
)

FIND_LIBRARY(AUDIERE_LIBRARY 
  NAMES audiere
  HINTS
  PATH_SUFFIXES lib64 lib libs64 libs libs/Win32 libs/Win64
  PATHS
  $ENV{AUDIERE_DIR}
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
  /sw
  /opt/local
  /opt/csw
  /opt
)

SET(AUDIERE_FOUND "NO")
IF(AUDIERE_LIBRARY AND AUDIERE_INCLUDE_DIR)
  SET(AUDIERE_FOUND "YES")
ENDIF(AUDIERE_LIBRARY AND AUDIERE_INCLUDE_DIR)

