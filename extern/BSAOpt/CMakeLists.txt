cmake_minimum_required(VERSION 2.8)

# This is NOT intended as a stand-alone build system! Instead, you should include this from the main CMakeLists of your project.

set(BSAOPTHASH_LIBRARY "bsaopthash")

# Sources
set(SOURCE_FILES
    hash.cpp
)

add_library(${BSAOPTHASH_LIBRARY} STATIC ${SOURCE_FILES})

set(BSAOPTHASH_LIBRARIES ${BSAOPTHASH_LIBRARY})

link_directories(${CMAKE_CURRENT_BINARY_DIR})
set(BSAOPTHASH_LIBRARIES ${BSAOPTHASH_LIBRARIES} PARENT_SCOPE)
