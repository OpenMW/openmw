# Try to find CXXOPTS library

# Once done this will define
#  CXXOPTS_FOUND         - System has the all required components.
#  CXXOPTS_INCLUDE_DIRS  - Include directory necessary for using the required components headers.
#  CXXOPTS_LIBRARIES     - Link these to use CXXOPTS.
#

include(LibFindMacros)

libfind_pkg_detect(CXXOPTS cxxopts
        FIND_PATH cxxopts.hpp
        )
libfind_process(CXXOPTS)
