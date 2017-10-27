# FindFmt
# -------
# Finds the Fmt library
#
# This will define the following variables::
#
# FMT_FOUND - system has Fmt
# FMT_INCLUDE_DIRS - the Fmt include directory
# FMT_LIBRARIES - the Fmt libraries
#
# and the following imported targets::
#
#   Fmt::Fmt   - The Fmt library


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

libfind_pkg_detect(FMT_LIBRARY libfmt
        FIND_PATH libfmt.h
        HINTS ${POSSIBLE_LOCATIONS}
        FIND_LIBRARY fmt
        HINTS ${POSSIBLE_LOCATIONS}
        )
libfind_process(Fmt)
