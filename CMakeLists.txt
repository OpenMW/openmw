# We probably support older versions than this.
cmake_minimum_required(VERSION 2.6)

# Local files
set(BSA bsa/bsa_archive.cpp bsa/bsa_file.cpp)
set(NIF nif/nif_file.cpp nifogre/ogre_nif_loader.cpp)
set(TOOLS tools/stringops.cpp)
set(MANGLE_VFS mangle/vfs/servers/ogre_vfs.cpp)
set(GAME game/main.cpp)

# Dependencies
# Not exactly platform independent, FIXME!
add_definitions(-I/usr/local/include/OGRE)

# Main executable
add_executable(openmw ${BSA} ${TOOLS} ${GAME})
target_link_libraries(openmw OgreMain)
