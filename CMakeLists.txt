project(OpenMW)

# We probably support older versions than this.
cmake_minimum_required(VERSION 2.6)

# Add path for CMake scripts
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/)

# source directory: apps

set(GAME 
    apps/openmw/main.cpp 
    apps/openmw/engine.cpp
    apps/openmw/world.cpp)
set(GAME_HEADER 
    apps/openmw/engine.hpp
    apps/openmw/world.hpp
    apps/openmw/refdata.hpp)
source_group(game FILES ${GAME} ${GAME_HEADER})

set(GAMEREND 
    apps/openmw/mwrender/mwscene.cpp 
    apps/openmw/mwrender/cellimp.cpp
    apps/openmw/mwrender/interior.cpp
    apps/openmw/mwrender/sky.cpp)
set(GAMEREND_HEADER 
    apps/openmw/mwrender/cell.hpp 
    apps/openmw/mwrender/cellimp.hpp 
    apps/openmw/mwrender/mwscene.hpp
    apps/openmw/mwrender/interior.hpp 
    apps/openmw/mwrender/playerpos.hpp
    apps/openmw/mwrender/sky.hpp)
source_group(apps\\openmw\\mwrender FILES ${GAMEREND} ${GAMEREND_HEADER})    

# set(GAMEINPUT)
set(GAMEINPUT_HEADER 
    apps/openmw/mwinput/inputmanager.hpp)
source_group(apps\\openmw\\mwinput FILES ${GAMEINPUT} ${GAMEINPUT_HEADER})    

set(GAMESCRIPT
    apps/openmw/mwscript/scriptmanager.cpp
    apps/openmw/mwscript/interpretercontext.cpp
    apps/openmw/mwscript/cellextensions.cpp
    apps/openmw/mwscript/extensions.cpp)
set(GAMESCRIPT_HEADER 
    apps/openmw/mwscript/locals.hpp
    apps/openmw/mwscript/scriptmanager.hpp
    apps/openmw/mwscript/compilercontext.hpp
    apps/openmw/mwscript/compilercontextscript.hpp
    apps/openmw/mwscript/interpretercontext.hpp
    apps/openmw/mwscript/cellextensions.hpp
    apps/openmw/mwscript/extensions.hpp)
source_group(apps\\openmw\\mwscript FILES ${GAMESCRIPT} ${GAMESCRIPT_HEADER})    

set(GAMESOUND
    apps/openmw/mwsound/soundmanager.cpp
    apps/openmw/mwsound/extensions.cpp)
set(GAMESOUND_HEADER 
    apps/openmw/mwsound/soundmanager.hpp
    apps/openmw/mwsound/extensions.hpp)
source_group(apps\\openmw\\mwsound FILES ${GAMESOUND} ${GAMESOUND_HEADER}) 

set(APPS ${GAME} ${GAMEREND} ${GAMEINPUT} ${GAMESCRIPT} ${GAMESOUND})
set(APPS_HEADER ${GAME_HEADER} ${GAMEREND_HEADER} ${GAMEINPUT_HEADER} ${GAMESCRIPT_HEADER}
    ${GAMESOUND_HEADER})

# source directory: components

set(BSA 
    components/bsa/bsa_archive.cpp 
    components/bsa/bsa_file.cpp)
set(BSA_HEADER 
    components/bsa/bsa_archive.hpp 
    components/bsa/bsa_file.hpp)
source_group(components\\bsa FILES ${BSA} ${BSA_HEADER})

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
source_group(components\\nif FILES ${NIF} ${NIF_HEADER})    

set(NIFOGRE 
    components/nifogre/ogre_nif_loader.cpp)
set(NIFOGRE_HEADER 
    components/nifogre/ogre_nif_loader.hpp)
source_group(components\\nifogre FILES ${NIFOGRE} ${NIFOGRE_HEADER})    

set(ESM_STORE 
    components/esm_store/store.cpp)
set(ESM_STORE_HEADER 
    components/esm_store/cell_store.hpp
    components/esm_store/reclists.hpp 
    components/esm_store/store.hpp)
source_group(components\\esm_store FILES ${ESM_STORE} ${ESM_STORE_HEADER})

file(GLOB ESM_HEADER components/esm/*.hpp)
source_group(components\\esm FILES ${ESM_HEADER}) 

set(OGRE 
    components/engine/ogre/renderer.cpp)
set(OGRE_HEADER 
    components/engine/ogre/renderer.hpp)
source_group(components\\engine\\ogre FILES ${OGRE} ${OGRE_HEADER})

set(INPUT 
    components/engine/input/oismanager.cpp)
set(INPUT_HEADER 
    components/engine/input/oismanager.hpp 
    components/engine/input/listener.hpp
    components/engine/input/func_binder.hpp 
    components/engine/input/dispatch_map.hpp
    components/engine/input/dispatcher.hpp 
    components/engine/input/poller.hpp)
source_group(components\\engine\\input FILES ${INPUT} ${INPUT_HEADER})

set(COMMANDSERVER
    components/commandserver/command.hpp
    components/commandserver/server.hpp
    components/commandserver/server.cpp)
source_group(components\\commandserver FILES ${COMMANDSERVER})    

set(MISC 
    components/misc/stringops.cpp 
    components/misc/fileops.cpp)
set(MISC_HEADER 
    components/misc/fileops.hpp 
    components/misc/slice_array.hpp
    components/misc/stringops.hpp
    components/misc/tsdeque.hpp)
source_group(components\\misc FILES ${MISC} ${MISC_HEADER})   
    
file(GLOB COMPILER components/compiler/*.cpp)
file(GLOB COMPILER_HEADER components/compiler/*.hpp)
source_group(components\\compiler FILES ${COMPILER} ${COMPILER_HEADER})    

file(GLOB INTERPRETER components/interpreter/*.cpp)
file(GLOB INTERPRETER_HEADER components/interpreter/*.hpp)
source_group(components\\interpreter FILES ${INTERPRETER} ${INTERPRETER_HEADER})    

set(COMPONENTS ${BSA} ${NIF} ${NIFOGRE} ${ESM_STORE} ${OGRE} ${INPUT} ${MISC} 
    ${COMMANDSERVER}
    ${COMPILER}
    ${INTERPRETER})
set(COMPONENTS_HEADER ${BSA_HEADER} ${NIF_HEADER} ${NIFOGRE_HEADER} ${ESM_STORE_HEADER}
    ${ESM_HEADER} ${OGRE_HEADER} ${INPUT_HEADER} ${MISC_HEADER} ${COMPILER_HEADER}
    ${INTERPRETER_HEADER})

# source directory: libs

set(MANGLE_VFS libs/mangle/vfs/servers/ogre_vfs.cpp)
source_group(libs\\mangle_vfs FILES ${MANGLE_VFS})

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

add_subdirectory( extern/caelum )

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

# Other apps and tools
add_subdirectory( apps/clientconsole )

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

# Tools
option(BUILD_MWCOMPILER "build standalone Morrowind script compiler" ON)
if (BUILD_MWCOMPILER)
    add_subdirectory( apps/mwcompiler )
endif()

option(BUILD_MWINTERPRETER "build standalone Morrowind script code interpreter" ON)
if (BUILD_MWINTERPRETER)
    add_subdirectory( apps/mwinterpreter )
endif()
