project(Platform)

set(SOURCES 
    fileops.cpp
    fileops.hpp
    slice_array.hpp
    stdint.h
    stringops.cpp
    stringops.hpp
    strings.h)    
add_library(platform STATIC ${SOURCES})