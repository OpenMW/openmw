project(OpenMW)

# We probably support older versions than this.
cmake_minimum_required(VERSION 2.6)

# Add path for CMake scripts
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/)

# Local files

set(BSA bsa/bsa_archive.cpp bsa/bsa_file.cpp)
set(BSA_HEADER bsa/bsa_archive.hpp bsa/bsa_file.hpp)

set(NIF nif/nif_file.cpp)
set(NIF_HEADER nif/controlled.hpp nif/effect.hpp nif/nif_types.hpp  nif/record.hpp
    nif/controller.hpp nif/extra.hpp nif/node.hpp nif/record_ptr.hpp
    nif/data.hpp nif/nif_file.hpp nif/property.hpp)

set(NIFOGRE nifogre/ogre_nif_loader.cpp)
set(NIFOGRE_HEADER nifogre/ogre_nif_loader.hpp)

set(TOOLS tools/stringops.cpp tools/fileops.cpp)
set(TOOLS_HEADER tools/fileops.hpp tools/slice_array.hpp tools/stringops.hpp)

set(MANGLE_VFS mangle/vfs/servers/ogre_vfs.cpp)

set(OGRE ogre/renderer.cpp)
set(OGRE_HEADER ogre/renderer.hpp)

set(INPUT input/oismanager.cpp)
set(INPUT_HEADER input/oismanager.hpp input/listener.hpp input/func_binder.hpp input/dispatch_map.hpp input/dispatcher.hpp)

set(GAME game/main.cpp)
set(GAME_HEADER game/mwinput/inputmanager.hpp)

set(ESM_STORE esm_store/store.cpp esm_store/cell_store.cpp)
set(ESM_STORE_HEADER esm_store/cell_store.hpp esm_store/reclists.hpp esm_store/store.hpp)

set(GAMEREND game/mwrender/mwscene.cpp game/mwrender/cell.cpp game/mwrender/interior.cpp)
set(GAMEREND_HEADER game/mwrender/cell.hpp game/mwrender/mwscene.hpp
    game/mwrender/interior.hpp)

set(ESM_HEADER esm/defs.hpp esm/loadcell.hpp esm/loadfact.hpp esm/loadltex.hpp
    esm/loadskil.hpp
    esm/esm_reader.hpp esm/loadclas.hpp esm/loadglob.hpp esm/loadmgef.hpp esm/loadsndg.hpp
    esm/loadacti.hpp esm/loadclot.hpp esm/loadgmst.hpp esm/loadmisc.hpp esm/loadsoun.hpp
    esm/loadalch.hpp esm/loadcont.hpp esm/loadinfo.hpp esm/loadnpcc.hpp esm/loadspel.hpp
    esm/loadappa.hpp esm/loadcrea.hpp esm/loadingr.hpp esm/loadnpc.hpp esm/loadsscr.hpp
    esm/loadarmo.hpp esm/loadcrec.hpp esm/loadland.hpp esm/loadpgrd.hpp esm/loadstat.hpp
    esm/loadbody.hpp esm/loaddial.hpp esm/loadlevlist.hpp esm/loadrace.hpp esm/loadweap.hpp
    esm/loadbook.hpp esm/loaddoor.hpp esm/loadligh.hpp esm/loadregn.hpp esm/records.hpp
    esm/loadbsgn.hpp esm/loadench.hpp esm/loadlocks.hpp esm/loadscpt.hpp)

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
    ${PLATFORM_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIRS} ${OGRE_LIB_DIR})

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

# Main executable
add_executable(openmw
    MACOSX_BUNDLE
    ${BSA} ${BSA_HEADER}
    ${TOOLS} ${TOOLS_HEADER}
    ${OGRE} ${OGRE_HEADER}
    ${INPUT} ${INPUT_HEADER}
    ${NIF} ${NIF_HEADER}
    ${NIFOGRE} ${NIFOGRE_HEADER}
    ${MANGLE_VFS}
    ${GAME}
    ${ESM_STORE} ${ESM_STORE_HEADER}
    ${GAMEREND} ${GAMEREND_HEADER}
    ${ESM_HEADER}
    ${APPLE_BUNDLE_RESOURCES}
    )

target_link_libraries(openmw
  ${OGRE_LIBRARIES}
  ${OIS_LIBRARIES}
  ${Boost_LIBRARIES})

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
