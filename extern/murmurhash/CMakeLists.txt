cmake_minimum_required(VERSION 2.8)

# This is NOT intended as a stand-alone build system! Instead, you should include this from the main CMakeLists of your project.

set(MURMURHASH_LIBRARY "murmurhash")

# Sources
set(SOURCE_FILES
    MurmurHash2.cpp
)

add_library(${MURMURHASH_LIBRARY} STATIC ${SOURCE_FILES})

set(MURMURHASH_LIBRARIES ${MURMURHASH_LIBRARY})

link_directories(${CMAKE_CURRENT_BINARY_DIR})
set(MURMURHASH_LIBRARIES ${MURMURHASH_LIBRARIES} PARENT_SCOPE)
