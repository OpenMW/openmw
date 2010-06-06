project(OpenMW)

# We probably support older versions than this.
cmake_minimum_required(VERSION 2.6)

# Add path for CMake scripts
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/)

# Local files
set(BSA bsa/bsa_archive.cpp bsa/bsa_file.cpp)
set(NIF nif/nif_file.cpp nifogre/ogre_nif_loader.cpp)
set(TOOLS tools/stringops.cpp tools/fileops.cpp)
set(MANGLE_VFS mangle/vfs/servers/ogre_vfs.cpp)
set(OGRE ogre/renderer.cpp)
set(INPUT input/oismanager.cpp)
set(GAME game/main.cpp game/esm_store/store.cpp game/cell_store.cpp)
set(GAMEREND game/render/mwscene.cpp)

# Platform specific
if (WIN32)
set(PLATFORM_INCLUDE_DIR "platform")
else (WIN32)
set(PLATFORM_INCLUDE_DIR "")
endif (WIN32)

# Dependencies
find_package(OGRE REQUIRED)
find_package(Boost REQUIRED)
find_package(OIS REQUIRED)
include_directories("." ${OGRE_INCLUDE_DIR} ${Boost_INCLUDE_DIR} ${PLATFORM_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIRS} ${OGRE_LIB_DIR})

# Main executable
add_executable(openmw ${BSA} ${TOOLS} ${OGRE} ${INPUT} ${GAME} ${GAMEREND})
target_link_libraries(openmw ${OGRE_LIBRARIES} ${OIS_LIBRARIES})
