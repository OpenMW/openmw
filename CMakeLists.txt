project(OpenMW)

# We probably support older versions than this.
cmake_minimum_required(VERSION 2.6)

# Add path for CMake scripts
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/)

# source directory: apps

set(GAME 
    apps/openmw/main.cpp 
    apps/openmw/engine.cpp)
set(GAME_HEADER 
    apps/openmw/mwinput/inputmanager.hpp 
    apps/openmw/engine.hpp)
source_group(game FILES ${GAME} ${GAME_HEADER})

set(GAMEREND 
    apps/openmw/mwrender/mwscene.cpp 
    apps/openmw/mwrender/cell.cpp
    apps/openmw/mwrender/interior.cpp
    apps/openmw/mwrender/sky.cpp)
set(GAMEREND_HEADER 
    apps/openmw/mwrender/cell.hpp 
    apps/openmw/mwrender/mwscene.hpp
    apps/openmw/mwrender/interior.hpp 
    apps/openmw/mwrender/playerpos.hpp
    apps/openmw/mwrender/sky.hpp)
source_group(game_renderer FILES ${GAMEREND} ${GAMEREND_HEADER})    

# set(GAMEINPUT)
set(GAMEINPUT_HEADER 
    apps/openmw/mwinput/inputmanager.hpp)
source_group(game_input FILES ${GAMEINPUT} ${GAMEINPUT_HEADER})    

set(APPS ${GAME} ${GAMEREND} ${GAMEINPUT})
set(APPS_HEADER ${GAME_HEADER} ${GAMEREND_HEADER} ${GAMEINPUT_HEADER})

# source directory: components

set(BSA 
    components/bsa/bsa_archive.cpp 
    components/bsa/bsa_file.cpp)
set(BSA_HEADER 
    components/bsa/bsa_archive.hpp 
    components/bsa/bsa_file.hpp)
source_group(bsa FILES ${BSA} ${BSA_HEADER})

set(NIF 
    components/nif/nif_file.cpp)
set(NIF_HEADER 
    components/nif/controlled.hpp 
    components/nif/effect.hpp
    components/nif/nif_types.hpp  
    components/nif/record.hpp
    components/nif/controller.hpp 
    components/nif/extra.hpp 
    components/nif/node.hpp
    components/nif/record_ptr.hpp
    components/nif/data.hpp 
    components/nif/nif_file.hpp 
    components/nif/property.hpp)
source_group(nif FILES ${NIF} ${NIF_HEADER})    

set(NIFOGRE 
    components/nifogre/ogre_nif_loader.cpp)
set(NIFOGRE_HEADER 
    components/nifogre/ogre_nif_loader.hpp)
source_group(nifogre FILES ${NIFOGRE} ${NIFOGRE_HEADER})    

set(ESM_STORE 
    components/esm_store/store.cpp 
    components/esm_store/cell_store.cpp)
set(ESM_STORE_HEADER 
    components/esm_store/cell_store.hpp
    components/esm_store/reclists.hpp 
    components/esm_store/store.hpp)
source_group(esm_store FILES ${ESM_STORE} ${ESM_STORE_HEADER})

file(GLOB ESM_HEADER components/esm/*.hpp)
source_group(esm_header FILES ${ESM_HEADER}) 

set(OGRE 
    components/engine/ogre/renderer.cpp)
set(OGRE_HEADER 
    components/engine/ogre/renderer.hpp)
source_group(ogre FILES ${OGRE} ${OGRE_HEADER})

set(INPUT 
    components/engine/input/oismanager.cpp)
set(INPUT_HEADER 
    components/engine/input/oismanager.hpp 
    components/engine/input/listener.hpp
    components/engine/input/func_binder.hpp 
    components/engine/input/dispatch_map.hpp
    components/engine/input/dispatcher.hpp 
    components/engine/input/poller.hpp)
source_group(input FILES ${INPUT} ${INPUT_HEADER})

set(MISC 
    components/misc/stringops.cpp 
    components/misc/fileops.cpp)
set(MISC_HEADER 
    components/misc/fileops.hpp 
    components/misc/slice_array.hpp
    components/misc/stringops.hpp)
source_group(misc FILES ${MISC} ${MISC_HEADER})   

set(COMPONENTS ${BSA} ${NIF} ${NIFOGRE} ${ESM_STORE} ${OGRE} ${INPUT} ${MISC})
set(COMPONENTS_HEADER ${BSA_HEADER} ${NIF_HEADER} ${NIFOGRE_HEADER} ${ESM_STORE_HEADER}
    ${ESM_HEADER} ${OGRE_HEADER} ${INPUT_HEADER} ${MISC_HEADER})
    
# source directory: libs

set(MANGLE_VFS libs/mangle/vfs/servers/ogre_vfs.cpp)
source_group(mangle_vfs FILES ${MANGLE_VFS})

set(OPENMW_LIBS ${MANGLE_VFS})
set(OPENMW_LIBS_HEADER)
    
# Platform specific
if (WIN32)
set(PLATFORM_INCLUDE_DIR "platform")
else (WIN32)
set(PLATFORM_INCLUDE_DIR "")
endif (WIN32)

# Dependencies

find_package(OGRE REQUIRED)
find_package(Boost REQUIRED COMPONENTS system filesystem program_options)
find_package(OIS REQUIRED)
include_directories("."
    ${OGRE_INCLUDE_DIR} ${OIS_INCLUDE_DIR} ${Boost_INCLUDE_DIR}
    ${PLATFORM_INCLUDE_DIR}
    ${CMAKE_HOME_DIRECTORY}/extern/caelum/include)
link_directories(${Boost_LIBRARY_DIRS} ${OGRE_LIB_DIR})

ADD_SUBDIRECTORY( extern/caelum )

# Specify build paths

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${OpenMW_BINARY_DIR}")

# Other files

if (WIN32)
configure_file(${OpenMW_SOURCE_DIR}/files/plugins.cfg.win32
    "${OpenMW_BINARY_DIR}/plugins.cfg" COPYONLY)
else (WIN32)
configure_file(${OpenMW_SOURCE_DIR}/files/plugins.cfg.linux
    "${OpenMW_BINARY_DIR}/plugins.cfg" COPYONLY)
endif (WIN32)

configure_file(${OpenMW_SOURCE_DIR}/files/openmw.cfg
    "${OpenMW_BINARY_DIR}/openmw.cfg" COPYONLY)

if (APPLE)
  set(APPLE_BUNDLE_RESOURCES
    ${CMAKE_SOURCE_DIR}/files/openmw.cfg
    ${CMAKE_SOURCE_DIR}/files/mac/plugins.cfg
  )
endif (APPLE)

# Compiler settings
if (CMAKE_COMPILER_IS_GNUCC)
    add_definitions (-Wall)
endif (CMAKE_COMPILER_IS_GNUCC)

# Main executable
add_executable(openmw
    MACOSX_BUNDLE
    ${COMPONENTS} ${COMPONENTS_HEADER}
    ${OPENMW_LIBS} ${OPENMW_LIBS_HEADER}
    ${APPS} ${APPS_HEADER}
    ${ESM_HEADER}
    ${APPLE_BUNDLE_RESOURCES}
    )

target_link_libraries(openmw
  ${OGRE_LIBRARIES}
  ${OIS_LIBRARIES}
  ${Boost_LIBRARIES}
  caelum)

if (APPLE)
    find_library(CARBON_FRAMEWORK Carbon)
    target_link_libraries(openmw ${CARBON_FRAMEWORK})
endif (APPLE)

# Apple bundling
if (APPLE)
  set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/files/openmw.cfg
    ${CMAKE_SOURCE_DIR}/files/mac/plugins.cfg
    PROPERTIES
    MACOSX_PACKAGE_LOCATION MacOS
  )
  set_target_properties(
    openmw
    PROPERTIES
    MACOSX_BUNDLE_BUNDLE_NAME "OpenMW"
  )
endif (APPLE)
