project(Platform)

file(GLOB_RECURSE SOURCES src/*)
file(GLOB_RECURSE CAELUM_HDR include/*)

set(SOURCES 
    fileops.cpp
    fileops.hpp
    slice_array.hpp
    stdint.h
    stringops.cpp
    stringops.hpp
    strings.h)    
add_library(platform STATIC ${SOURCES})