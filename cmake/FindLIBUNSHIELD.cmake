# Locate LIBUNSHIELD
# This module defines
# LIBUNSHIELD_LIBRARIES
# LIBUNSHIELD_FOUND, if false, do not try to link to LibUnshield
# LIBUNSHIELD_INCLUDE_DIRS, where to find the headers
#
# Created by Tom Mason (wheybags) for OpenMW (https://openmw.org), based on FindMPG123.cmake
#
# Ripped off from other sources. In fact, this file is so generic (I
# just did a search and replace on another file) that I wonder why the
# CMake guys haven't wrapped this entire thing in a single
# function. Do we really need to repeat this stuff for every single
# library when they all work the same? </today's rant>

include(LibFindMacros)

set(POSSIBLE_LOCATIONS
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

libfind_pkg_detect(LIBUNSHIELD libunshield
    FIND_PATH libunshield.h
        HINTS ${POSSIBLE_LOCATIONS}
    FIND_LIBRARY unshield
        HINTS ${POSSIBLE_LOCATIONS}
)
libfind_process(LIBUNSHIELD)
