project(OpenMW)

# We probably support older versions than this.
cmake_minimum_required(VERSION 2.6)

# Add path for CMake scripts
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/)

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

set(MISC 
    ${COMP_DIR}/misc/stringops.cpp
    ${COMP_DIR}/misc/fileops.cpp)
set(MISC_HEADER 
    ${COMP_DIR}/misc/fileops.hpp
    ${COMP_DIR}/misc/slice_array.hpp
    ${COMP_DIR}/misc/stringops.hpp)
source_group(components\\misc FILES ${MISC} ${MISC_HEADER})   
    
file(GLOB COMPILER ${COMP_DIR}/compiler/*.cpp)
file(GLOB COMPILER_HEADER ${COMP_DIR}/compiler/*.hpp)
source_group(components\\compiler FILES ${COMPILER} ${COMPILER_HEADER})    

file(GLOB INTERPRETER ${COMP_DIR}/interpreter/*.cpp)
file(GLOB INTERPRETER_HEADER ${COMP_DIR}/interpreter/*.hpp)
source_group(components\\interpreter FILES ${INTERPRETER} ${INTERPRETER_HEADER})    

set(COMPONENTS ${BSA} ${NIF} ${NIFOGRE} ${ESM_STORE} ${MISC}
    ${COMPILER} ${INTERPRETER})
set(COMPONENTS_HEADER ${BSA_HEADER} ${NIF_HEADER} ${NIFOGRE_HEADER} ${ESM_STORE_HEADER}
    ${ESM_HEADER} ${MISC_HEADER} ${COMPILER_HEADER}
    ${INTERPRETER_HEADER})

# source directory: libs

set(LIBDIR ${CMAKE_SOURCE_DIR}/libs)

set(MANGLE_VFS ${LIBDIR}/mangle/vfs/servers/ogre_vfs.cpp)
set(MANGLE_INPUT ${LIBDIR}/mangle/input/servers/ois_driver.cpp)
set(MANGLE_ALL ${MANGLE_VFS} ${MANGLE_INPUT})
source_group(libs\\mangle FILES ${MANGLE_ALL})

set(OENGINE_OGRE
  ${LIBDIR}/openengine/ogre/renderer.cpp
  ${LIBDIR}/openengine/ogre/mouselook.cpp
)
set(OENGINE_GUI
  ${LIBDIR}/openengine/gui/events.cpp
  ${LIBDIR}/openengine/gui/manager.cpp
)
set(OENGINE_ALL ${OENGINE_OGRE} ${OENGINE_GUI})
source_group(libs\\openengine FILES ${OENGINE_ALL})

set(OPENMW_LIBS ${MANGLE_ALL} ${OENGINE_ALL})
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
    ${LIBDIR}
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

# Apps and tools
add_subdirectory( apps/openmw )

option(BUILD_ESMTOOL "build ESM inspector" ON)
if (BUILD_ESMTOOL)
  add_subdirectory( apps/esmtool )
endif()
