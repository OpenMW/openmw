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
    apps/openmw/engine.hpp)
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
    apps/openmw/mwscript/compilercontext.cpp
    apps/openmw/mwscript/interpretercontext.cpp
    apps/openmw/mwscript/cellextensions.cpp
    apps/openmw/mwscript/miscextensions.cpp
    apps/openmw/mwscript/guiextensions.cpp
    apps/openmw/mwscript/extensions.cpp
    apps/openmw/mwscript/globalscripts.cpp
    )
set(GAMESCRIPT_HEADER 
    apps/openmw/mwscript/locals.hpp
    apps/openmw/mwscript/scriptmanager.hpp
    apps/openmw/mwscript/compilercontext.hpp
    apps/openmw/mwscript/interpretercontext.hpp
    apps/openmw/mwscript/cellextensions.hpp
    apps/openmw/mwscript/miscextensions.hpp
    apps/openmw/mwscript/guiextensions.hpp
    apps/openmw/mwscript/extensions.hpp
    apps/openmw/mwscript/globalscripts.hpp
    )
source_group(apps\\openmw\\mwscript FILES ${GAMESCRIPT} ${GAMESCRIPT_HEADER})    

set(GAMESOUND
    apps/openmw/mwsound/soundmanager.cpp
    apps/openmw/mwsound/extensions.cpp)
set(GAMESOUND_HEADER 
    apps/openmw/mwsound/soundmanager.hpp
    apps/openmw/mwsound/extensions.hpp)
source_group(apps\\openmw\\mwsound FILES ${GAMESOUND} ${GAMESOUND_HEADER}) 

set(GAMEGUI
    apps/openmw/mwgui/guimanager.cpp)
set(GAMEGUI_HEADER 
    apps/openmw/mwgui/guimanager.hpp)
source_group(apps\\openmw\\mwgui FILES ${GAMEGUI} ${GAMEGUI_HEADER}) 

set(GAMEWORLD
    apps/openmw/mwworld/world.cpp)
set(GAMEWORLD_HEADER 
    apps/openmw/mwworld/refdata.hpp
    apps/openmw/mwworld/world.hpp
    apps/openmw/mwworld/ptr.hpp
    apps/openmw/mwworld/environment.hpp
    )
source_group(apps\\openmw\\mwworld FILES ${GAMEWORLD} ${GAMEWORLD_HEADER}) 


set(APPS ${GAME} ${GAMEREND} ${GAMEINPUT} ${GAMESCRIPT} ${GAMESOUND} ${GAMEGUI} ${GAMEWORLD})
set(APPS_HEADER ${GAME_HEADER} ${GAMEREND_HEADER} ${GAMEINPUT_HEADER} ${GAMESCRIPT_HEADER}
    ${GAMESOUND_HEADER} ${GAMEGUI_HEADER} ${GAMEWORLD_HEADER})

# source directory: components
set(COMP_DIR ${CMAKE_SOURCE_DIR}/components)

set(BSA 
    ${COMP_DIR}/bsa/bsa_archive.cpp
    ${COMP_DIR}/bsa/bsa_file.cpp)
set(BSA_HEADER 
    ${COMP_DIR}/bsa/bsa_archive.hpp
    ${COMP_DIR}/bsa/bsa_file.hpp)
source_group(components\\bsa FILES ${BSA} ${BSA_HEADER})

set(NIF 
    ${COMP_DIR}/nif/nif_file.cpp)
set(NIF_HEADER 
    ${COMP_DIR}/nif/controlled.hpp
    ${COMP_DIR}/nif/effect.hpp
    ${COMP_DIR}/nif/nif_types.hpp
    ${COMP_DIR}/nif/record.hpp
    ${COMP_DIR}/nif/controller.hpp
    ${COMP_DIR}/nif/extra.hpp
    ${COMP_DIR}/nif/node.hpp
    ${COMP_DIR}/nif/record_ptr.hpp
    ${COMP_DIR}/nif/data.hpp
    ${COMP_DIR}/nif/nif_file.hpp
    ${COMP_DIR}/nif/property.hpp)
source_group(components\\nif FILES ${NIF} ${NIF_HEADER})    

set(NIFOGRE 
    ${COMP_DIR}/nifogre/ogre_nif_loader.cpp)
set(NIFOGRE_HEADER 
    ${COMP_DIR}/nifogre/ogre_nif_loader.hpp)
source_group(components\\nifogre FILES ${NIFOGRE} ${NIFOGRE_HEADER})    

set(ESM_STORE 
    ${COMP_DIR}/esm_store/store.cpp)
set(ESM_STORE_HEADER 
    ${COMP_DIR}/esm_store/cell_store.hpp
    ${COMP_DIR}/esm_store/reclists.hpp
    ${COMP_DIR}/esm_store/store.hpp)
source_group(components\\esm_store FILES ${ESM_STORE} ${ESM_STORE_HEADER})

file(GLOB ESM_HEADER ${COMP_DIR}/esm/*.hpp)
source_group(components\\esm FILES ${ESM_HEADER}) 

set(OGRE 
    ${COMP_DIR}/engine/ogre/renderer.cpp)
set(OGRE_HEADER 
    ${COMP_DIR}/engine/ogre/renderer.hpp)
source_group(components\\engine\\ogre FILES ${OGRE} ${OGRE_HEADER})

set(INPUT 
    ${COMP_DIR}/engine/input/oismanager.cpp)
set(INPUT_HEADER 
    ${COMP_DIR}/engine/input/oismanager.hpp
    ${COMP_DIR}/engine/input/listener.hpp
    ${COMP_DIR}/engine/input/func_binder.hpp
    ${COMP_DIR}/engine/input/dispatch_map.hpp
    ${COMP_DIR}/engine/input/dispatcher.hpp
    ${COMP_DIR}/engine/input/poller.hpp)
source_group(components\\engine\\input FILES ${INPUT} ${INPUT_HEADER})

set(COMMANDSERVER
    ${COMP_DIR}/commandserver/command.hpp
    ${COMP_DIR}/commandserver/server.hpp
    ${COMP_DIR}/commandserver/server.cpp)
source_group(components\\commandserver FILES ${COMMANDSERVER})    

set(MISC 
    ${COMP_DIR}/misc/stringops.cpp
    ${COMP_DIR}/misc/fileops.cpp)
set(MISC_HEADER 
    ${COMP_DIR}/misc/fileops.hpp
    ${COMP_DIR}/misc/slice_array.hpp
    ${COMP_DIR}/misc/stringops.hpp
    ${COMP_DIR}/misc/tsdeque.hpp)
source_group(components\\misc FILES ${MISC} ${MISC_HEADER})   
    
file(GLOB COMPILER ${COMP_DIR}/compiler/*.cpp)
file(GLOB COMPILER_HEADER ${COMP_DIR}/compiler/*.hpp)
source_group(components\\compiler FILES ${COMPILER} ${COMPILER_HEADER})    

file(GLOB INTERPRETER ${COMP_DIR}/interpreter/*.cpp)
file(GLOB INTERPRETER_HEADER ${COMP_DIR}/interpreter/*.hpp)
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
    add_definitions(-DBOOST_ALL_NO_LIB)
else (WIN32)
set(PLATFORM_INCLUDE_DIR "")
endif (WIN32)

# Dependencies

find_package(OGRE REQUIRED)
find_package(Boost REQUIRED COMPONENTS system filesystem program_options thread)
find_package(OIS REQUIRED)
include_directories("."
    ${OGRE_INCLUDE_DIR} ${OGRE_INCLUDE_DIR}/Ogre
    ${OIS_INCLUDE_DIR} ${Boost_INCLUDE_DIR}
    ${PLATFORM_INCLUDE_DIR}
    ${CMAKE_HOME_DIRECTORY}/extern/caelum/include
    ${CMAKE_HOME_DIRECTORY}/extern/mygui_3.0.1/MyGUIEngine/include
    ${CMAKE_HOME_DIRECTORY}/extern/mygui_3.0.1/OgrePlatform/include
)


link_directories(${Boost_LIBRARY_DIRS} ${OGRE_LIB_DIR})

add_subdirectory( extern/caelum )
add_subdirectory( extern/mygui_3.0.1 )

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
# ???
#   MACOSX_BUNDLE
    ${COMPONENTS} ${COMPONENTS_HEADER}
    ${OPENMW_LIBS} ${OPENMW_LIBS_HEADER}
    ${APPS} ${APPS_HEADER}
    ${APPLE_BUNDLE_RESOURCES}
    )

target_link_libraries(openmw
  ${OGRE_LIBRARIES}
  ${OIS_LIBRARIES}
  ${Boost_LIBRARIES}
  caelum
  MyGUIEngine
  MyGUI.OgrePlatform
)

if (APPLE)
    find_library(CARBON_FRAMEWORK Carbon)
    target_link_libraries(openmw ${CARBON_FRAMEWORK})
endif (APPLE)

# Other apps and tools
add_subdirectory( apps/clientconsole )
add_subdirectory( apps/mygui_dev )

# Apple bundling
if (APPLE)
  set(MISC_FILES 
		  ${CMAKE_SOURCE_DIR}/files/openmw.cfg
		  ${CMAKE_SOURCE_DIR}/files/mac/plugins.cfg
		  ${CMAKE_SOURCE_DIR}/files/mac/ogre.cfg)
install(TARGETS openmw 
		BUNDLE DESTINATION .
		RUNTIME DESTINATION ../MacOS 
		COMPONENT Runtime)
install(FILES ${MISC_FILES} DESTINATION ../MacOS)
set(CPACK_GENERATOR "Bundle")
set(CPACK_BUNDLE_PLIST "${CMAKE_SOURCE_DIR}/files/mac/Info.plist")
set(CPACK_BUNDLE_ICON  "${CMAKE_SOURCE_DIR}/files/mac/openmw.icns")
set(CPACK_BUNDLE_NAME "OpenMW")
set(CPACK_PACKAGE_VERSION "0.07")
set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "07")
set(CPACK_PACKAGE_VERSION_PATCH "")

include(CPack)

set(CMAKE_EXE_LINKER_FLAGS "-arch i386")
set(CMAKE_CXX_FLAGS "-arch i386")

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
