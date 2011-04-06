project(OpenMW)

IF (APPLE)
    set(APP_BUNDLE_DIR "${OpenMW_BINARY_DIR}/OpenMW.app")
ENDIF (APPLE)

# Sound source selection
option(USE_AUDIERE "use Audiere for sound" OFF)
option(USE_FFMPEG "use ffmpeg for sound" OFF)
option(USE_MPG123 "use mpg123 + libsndfile for sound" ON)

find_program(DPKG_PROGRAM dpkg DOC "dpkg program of Debian-based systems")

# Location of morrowind data files
if(DPKG_PROGRAM)
    set(MORROWIND_DATA_FILES "/usr/share/games/openmw/data/" CACHE PATH "location of Morrowind data files")
    set(MORROWIND_RESOURCE_FILES "/usr/share/games/openmw/resources/" CACHE PATH "location of Morrowind data files")
else()
    if (APPLE)
        # set path inside bundle
        set(MORROWIND_DATA_FILES "../data" CACHE PATH "location of Morrowind data files")
        set(MORROWIND_RESOURCE_FILES "Contents/Resources/resources" CACHE PATH "location of Morrowind data files")
    else()
        set(MORROWIND_DATA_FILES "data" CACHE PATH "location of Morrowind data files")
        set(MORROWIND_RESOURCE_FILES "resources" CACHE PATH "location of Morrowind data files")
    endif(APPLE)
endif(DPKG_PROGRAM)

if (WIN32)
    option(USE_DEBUG_CONSOLE "whether a debug console should be enabled for debug builds, if false debug output is redirected to Visual Studio output" ON)
endif()

# We probably support older versions than this.
cmake_minimum_required(VERSION 2.6)

#
# Pre-built binaries being used?
#
IF(EXISTS "${CMAKE_SOURCE_DIR}/prebuilt/vc100-mt-gd/ogre_1_7_1")
    set(PREBUILT_DIR "${CMAKE_SOURCE_DIR}/prebuilt/vc100-mt-gd")
    message (STATUS "OpenMW pre-built binaries found at ${PREBUILT_DIR}.")

    SET(ENV{OGRE_HOME} "${PREBUILT_DIR}/ogre_1_7_1")

    SET(ENV{BOOST_ROOT} "${PREBUILT_DIR}/boost_1_42_0")
    set(Boost_USE_STATIC_LIBS   ON)
    set(Boost_USE_MULTITHREADED ON)
    set(ENV{BOOST_INCLUDEDIR} "${BOOST_ROOT}/include")
    set(ENV{BOOST_LIBRARYDIR} "${BOOST_ROOT}/lib")

    set(ENV{FREETYPE_DIR} "${PREBUILT_DIR}/freetype-2.3.5-1")

    set(USE_MPG123 OFF)
    set(USE_AUDIERE ON)
    set(AUDIERE_INCLUDE_DIR "${PREBUILT_DIR}/audiere-1.9.4/include")
    set(AUDIERE_LIBRARY "${PREBUILT_DIR}/audiere-1.9.4/lib/audiere.lib")

    set(ENV{OPENALDIR} "${PREBUILT_DIR}/OpenAL 1.1 SDK")

ELSE()
    message (STATUS "OpenMW pre-built binaries not found.  Using standard locations.")
ENDIF()

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

set(NIFBULLET
    ${COMP_DIR}/nifbullet/bullet_nif_loader.cpp)
set(NIFBULLET_HEADER
    ${COMP_DIR}/nifbullet/bullet_nif_loader.hpp)
source_group(components\\nifbullet FILES ${NIFBULLET} ${NIFBULLET_HEADER})

set(TO_UTF8
    ${COMP_DIR}/to_utf8/to_utf8.cpp)
set(TO_UTF8_HEADER
    ${COMP_DIR}/to_utf8/to_utf8.hpp)
source_group(components\\to_utf8 FILES ${TO_UTF8} ${TO_UTF8_HEADER})

set(FILE_FINDER
    ${COMP_DIR}/file_finder/search.cpp)
set(FILE_FINDER_HEADER
    ${COMP_DIR}/file_finder/file_finder.hpp
    ${COMP_DIR}/file_finder/filename_less.hpp
    ${COMP_DIR}/file_finder/search.hpp)
source_group(components\\file_finder FILES ${FILE_FINDER} ${FILE_FINDER_HEADER})

set(ESM_STORE
    ${COMP_DIR}/esm_store/store.cpp)
set(ESM_STORE_HEADER
    ${COMP_DIR}/esm_store/cell_store.hpp
    ${COMP_DIR}/esm_store/reclists.hpp
    ${COMP_DIR}/esm_store/store.hpp)
source_group(components\\esm_store FILES ${ESM_STORE} ${ESM_STORE_HEADER})

file(GLOB ESM_HEADER ${COMP_DIR}/esm/*.hpp)
set(ESM
    ${COMP_DIR}/esm/load_impl.cpp
    ${COMP_DIR}/esm/skill.cpp
    ${COMP_DIR}/esm/attr.cpp
    ${COMP_DIR}/esm/class.cpp
    ${COMP_DIR}/esm/esm_reader.cpp
    ${COMP_DIR}/esm/loadland.cpp
    ${COMP_DIR}/esm/loadacti.cpp
    ${COMP_DIR}/esm/loadalch.cpp
    ${COMP_DIR}/esm/loadappa.cpp
    ${COMP_DIR}/esm/loadarmo.cpp
    ${COMP_DIR}/esm/loadbody.cpp
    ${COMP_DIR}/esm/loadbook.cpp
    ${COMP_DIR}/esm/loadbsgn.cpp
    ${COMP_DIR}/esm/loadcell.cpp
    ${COMP_DIR}/esm/loadclas.cpp
    ${COMP_DIR}/esm/loadclot.cpp
    
)
source_group(components\\esm FILES ${ESM_HEADER} ${ESM})

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

set(COMPONENTS ${BSA} ${NIF} ${NIFOGRE} ${ESM_STORE} ${MISC} ${TO_UTF8}
    ${COMPILER} ${INTERPRETER} ${ESM} ${FILE_FINDER} ${NIFBULLET})
set(COMPONENTS_HEADER ${BSA_HEADER} ${NIF_HEADER} ${NIFOGRE_HEADER} ${ESM_STORE_HEADER}
    ${ESM_HEADER} ${MISC_HEADER} ${COMPILER_HEADER} ${TO_UTF8_HEADER}
    ${INTERPRETER_HEADER} ${FILE_FINDER_HEADER} ${NIFBULLET_HEADER})

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

set(OENGINE_BULLET
    ${LIBDIR}/openengine/bullet/btKinematicCharacterController.cpp
    ${LIBDIR}/openengine/bullet/btKinematicCharacterController.h
    ${LIBDIR}/openengine/bullet/BtOgre.cpp
    ${LIBDIR}/openengine/bullet/BtOgreExtras.h
    ${LIBDIR}/openengine/bullet/BtOgreGP.h
    ${LIBDIR}/openengine/bullet/BtOgrePG.h
    ${LIBDIR}/openengine/bullet/CMotionState.cpp
    ${LIBDIR}/openengine/bullet/CMotionState.h
    ${LIBDIR}/openengine/bullet/physic.cpp
    ${LIBDIR}/openengine/bullet/physic.hpp
    ${LIBDIR}/openengine/bullet/BulletShapeLoader.cpp
    ${LIBDIR}/openengine/bullet/BulletShapeLoader.h
)

# Sound setup
if (USE_AUDIERE)
    set(MANGLE_SOUND_OUTPUT
        ${LIBDIR}/mangle/sound/sources/audiere_source.cpp
        ${LIBDIR}/mangle/sound/sources/sample_reader.cpp
        ${LIBDIR}/mangle/stream/clients/audiere_file.cpp)
    find_package(Audiere REQUIRED)
    set(SOUND_INPUT_INCLUDES ${AUDIERE_INCLUDE_DIR})
    set(SOUND_INPUT_LIBRARY ${AUDIERE_LIBRARY})
    set(SOUND_DEFINE -DOPENMW_USE_AUDIERE)
endif (USE_AUDIERE)

if (USE_FFMPEG)
    set(MANGLE_SOUND_OUTPUT
        ${LIBDIR}/mangle/sound/sources/ffmpeg_source.cpp)
    find_package(FFMPEG REQUIRED)
    set(SOUND_INPUT_INCLUDES ${FFMPEG_INCLUDE_DIR})
    set(SOUND_INPUT_LIBRARY ${FFMPEG_LIBRARIES})
    set(SOUND_DEFINE -DOPENMW_USE_FFMPEG)
endif (USE_FFMPEG)

if (USE_MPG123)
    set(MANGLE_SOUND_OUTPUT
        ${LIBDIR}/mangle/sound/sources/mpg123_source.cpp
        ${LIBDIR}/mangle/sound/sources/libsndfile.cpp
        ${LIBDIR}/mangle/sound/sources/sample_reader.cpp)
    find_package(MPG123 REQUIRED)
    find_package(SNDFILE REQUIRED)
    set(SOUND_INPUT_INCLUDES ${MPG123_INCLUDE_DIR} ${SNDFILE_INCLUDE_DIR})
    set(SOUND_INPUT_LIBRARY ${MPG123_LIBRARY} ${SNDFILE_LIBRARY})
    set(SOUND_DEFINE -DOPENMW_USE_MPG123)
endif (USE_MPG123)

set(OENGINE_SOUND
  # Mangle and OEngine sound files are sort of intertwined, so put
  # them together here
  ${LIBDIR}/openengine/sound/sndmanager.cpp
  ${LIBDIR}/mangle/sound/outputs/openal_out.cpp
  ${MANGLE_SOUND_OUTPUT}
)
set(OENGINE_ALL ${OENGINE_OGRE} ${OENGINE_GUI} ${OENGINE_SOUND} ${OENGINE_BULLET})
source_group(libs\\openengine FILES ${OENGINE_ALL})

set(OPENMW_LIBS ${MANGLE_ALL} ${OENGINE_ALL})
set(OPENMW_LIBS_HEADER)

# Platform specific
if (WIN32)
    set(PLATFORM_INCLUDE_DIR "platform")
    add_definitions(-DBOOST_ALL_NO_LIB)
else (WIN32)
set(PLATFORM_INCLUDE_DIR "")
find_path (UUID_INCLUDE_DIR uuid/uuid.h)
include_directories(${UUID_INCLUDE_DIR})
endif (WIN32)
if (MSVC10)
    set(PLATFORM_INCLUDE_DIR "")
    add_definitions(-DMYGUI_DONT_REPLACE_NULLPTR)
endif()

if (APPLE)
    set(Boost_USE_STATIC_LIBS   ON)
endif (APPLE)

# Dependencies

find_package(OGRE REQUIRED)
find_package(Boost REQUIRED COMPONENTS system filesystem program_options thread)
find_package(OIS REQUIRED)
find_package(OpenAL REQUIRED)
find_package(Bullet REQUIRED)
include_directories("."
    ${OGRE_INCLUDE_DIR} ${OGRE_INCLUDE_DIR}/Ogre ${OGRE_INCLUDE_DIR}/OGRE
    ${OIS_INCLUDE_DIR} ${Boost_INCLUDE_DIR}
    ${PLATFORM_INCLUDE_DIR}
    ${CMAKE_HOME_DIRECTORY}/extern/caelum/include
    ${CMAKE_HOME_DIRECTORY}/extern/mygui_3.0.1/MyGUIEngine/include
    ${CMAKE_HOME_DIRECTORY}/extern/mygui_3.0.1/OgrePlatform/include
    ${OPENAL_INCLUDE_DIR}
    ${UUID_INCLUDE_DIR}
    ${LIBDIR}
)

link_directories(${Boost_LIBRARY_DIRS} ${OGRE_LIB_DIR})

add_subdirectory( extern/caelum )
add_subdirectory( extern/mygui_3.0.1 )

# Make sure that certain libraries are used as static libraries
# This is in effect turns off __declspec (dllexport) for windows
# Each library will also need to be configured to build as a static lib

# MyGUI: extern/mygui_3.0.0/
add_definitions(-DMYGUI_STATIC)

# Caelum: extern/caelum/
add_definitions(-DCAELUM_STATIC)

# Specify build paths

if (APPLE)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${APP_BUNDLE_DIR}/Contents/MacOS")
else (APPLE)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${OpenMW_BINARY_DIR}")
endif (APPLE)

# Other files

if (WIN32)
    configure_file(${OpenMW_SOURCE_DIR}/files/plugins.cfg.win32
        "${OpenMW_BINARY_DIR}/plugins.cfg" COPYONLY)
endif (WIN32)
if (LINUX)
    configure_file(${OpenMW_SOURCE_DIR}/files/plugins.cfg.linux
        "${OpenMW_BINARY_DIR}/plugins.cfg")
endif (LINUX)
if (APPLE)
    configure_file(${OpenMW_SOURCE_DIR}/files/plugins.cfg.mac
        "${APP_BUNDLE_DIR}/Contents/MacOS/plugins.cfg")
        
    configure_file(${OpenMW_SOURCE_DIR}/files/mac/Info.plist
        "${APP_BUNDLE_DIR}/Contents/Info.plist" COPYONLY)
    
    configure_file(${OpenMW_SOURCE_DIR}/files/mac/openmw.icns
        "${APP_BUNDLE_DIR}/Contents/Resources/OpenMW.icns" COPYONLY)

    
    # prepare plugins
    configure_file(${OGRE_PLUGIN_DIR}/RenderSystem_GL.dylib
        "${APP_BUNDLE_DIR}/Contents/Plugins/RenderSystem_GL.dylib" COPYONLY)
    
    configure_file(${OGRE_PLUGIN_DIR}/Plugin_OctreeSceneManager.dylib
        "${APP_BUNDLE_DIR}/Contents/Plugins/Plugin_OctreeSceneManager.dylib" COPYONLY)
        
    configure_file(${OGRE_PLUGIN_DIR}/Plugin_ParticleFX.dylib
        "${APP_BUNDLE_DIR}/Contents/Plugins/Plugin_ParticleFX.dylib" COPYONLY)
        
endif (APPLE)

configure_file(${OpenMW_SOURCE_DIR}/files/openmw.cfg
    "${APP_BUNDLE_DIR}/Contents/MacOS/openmw.cfg")

# Compiler settings
if (CMAKE_COMPILER_IS_GNUCC)
    #add_definitions (-Wall -Werror)
    add_definitions (-Wall)
endif (CMAKE_COMPILER_IS_GNUCC)

# Apple bundling
if (APPLE)
  set(MISC_FILES
        ${APP_BUNDLE_DIR}/Contents/MacOS/openmw.cfg
        ${APP_BUNDLE_DIR}/Contents/MacOS/plugins.cfg)
        
	set(OGRE_PLUGINS
		${APP_BUNDLE_DIR}/Contents/Plugins/*)
          
install(FILES ${MISC_FILES} DESTINATION ../MacOS)
install(DIRECTORY "${APP_BUNDLE_DIR}/Contents/Plugins" DESTINATION ..)
install(DIRECTORY "${APP_BUNDLE_DIR}/Contents/Resources/resources" DESTINATION ../Resources)
set(CPACK_GENERATOR "Bundle")
set(CPACK_BUNDLE_PLIST "${CMAKE_SOURCE_DIR}/files/mac/Info.plist")
set(CPACK_BUNDLE_ICON  "${CMAKE_SOURCE_DIR}/files/mac/openmw.icns")
set(CPACK_BUNDLE_NAME "OpenMW")
set(CPACK_PACKAGE_VERSION "0.10")
set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "10")
set(CPACK_PACKAGE_VERSION_PATCH "")

include(CPack)

set(CMAKE_EXE_LINKER_FLAGS "-arch i386")
set(CMAKE_CXX_FLAGS "-arch i386")

endif (APPLE)


if(DPKG_PROGRAM)
    SET(CMAKE_INSTALL_PREFIX "/usr")

    if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/.git")
        exec_program("git" ${CMAKE_CURRENT_SOURCE_DIR} ARGS "describe" OUTPUT_VARIABLE GIT_VERSION )
        STRING(REGEX REPLACE "openmw-" "" VERSION_STRING "${GIT_VERSION}")
        exec_program("git" ARGS "config --get user.name" OUTPUT_VARIABLE GIT_NAME )
        exec_program("git" ARGS "config --get user.email" OUTPUT_VARIABLE GIT_EMAIL)
        set(PACKAGE_MAINTAINER "${GIT_NAME} <${GIT_EMAIL}>")
    else()
        #FIXME this should probably be read from some file like ${CMAKE_CURRENT_SOURCE_DIR}/VERSION or something that gets updated when changing version
        set(VERSION_STRING "0.9.0")
        set(PACKAGE_MAINTAINER "unknown")
    endif()

    #Install global configuration files
    INSTALL(FILES "${OpenMW_BINARY_DIR}/openmw.cfg" DESTINATION "../etc/openmw/" PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ COMPONENT "openmw")
    INSTALL(FILES "${OpenMW_BINARY_DIR}/plugins.cfg" DESTINATION "../etc/openmw/" PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ COMPONENT "openmw")

    #Install resources
    INSTALL(DIRECTORY "${OpenMW_BINARY_DIR}/resources" DESTINATION "share/games/openmw/" FILE_PERMISSIONS OWNER_READ GROUP_READ WORLD_READ COMPONENT "Resources")
    INSTALL(DIRECTORY DESTINATION "share/games/openmw/data/" COMPONENT "Resources")

    SET(CPACK_GENERATOR "DEB")
    SET(CPACK_PACKAGE_NAME "openmw")
    SET(CPACK_DEBIAN_PACKAGE_HOMEPAGE "http://openmw.com")
    SET(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
    SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "${PACKAGE_MAINTAINER}")
    SET(CPACK_DEBIAN_PACKAGE_DESCRIPTION "A reimplementation of The Elder Scrolls III: Morrowind
 OpenMW is a reimplementation of the Bethesda Game Studios game The Elder Scrolls III: Morrowind.
 Data files from the original game is required to run it.")
    SET(CPACK_DEBIAN_PACKAGE_NAME "openmw")
    SET(CPACK_DEBIAN_PACKAGE_VERSION "${VERSION_STRING}")
    SET(CPACK_PACKAGE_EXECUTABLES "openmw;OpenMW")
    SET(CPACK_DEBIAN_PACKAGE_DEPENDS "libogremain-1.7.1 (>= 1.7.1-1),  libboost-filesystem1.42.0 (>= 1.42.0), libboost-program-options1.42.0 (>= 1.42.0), libboost-system1.42.0 (>= 1.42.0), libboost-thread1.42.0 (>= 1.42.0), libc6 (>= 2.11.2), libfreetype6 (>= 2.2.1), libgcc1 (>= 1:4.1.1), libmpg123-0 (>= 1.12.1), libois-1.2.0 (>= 1.2.0), libopenal1 (>= 1:1.12.854), libsndfile1 (>= 1.0.23), libstdc++6 (>= 4.4.5), libuuid1 (>= 2.17.2)")

    SET(CPACK_DEBIAN_PACKAGE_SECTION "Games")

    string(TOLOWER "${CPACK_PACKAGE_NAME}" CPACK_PACKAGE_NAME_LOWERCASE)
    execute_process(
        COMMAND ${DPKG_PROGRAM} --print-architecture
        OUTPUT_VARIABLE CPACK_DEBIAN_PACKAGE_ARCHITECTURE
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME_LOWERCASE}_${CPACK_DEBIAN_PACKAGE_VERSION}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")


    include(CPack)
endif(DPKG_PROGRAM)

# Apps and tools
add_subdirectory( apps/openmw )

option(BUILD_ESMTOOL "build ESM inspector" ON)
if (BUILD_ESMTOOL)
  add_subdirectory( apps/esmtool )
endif()

if (WIN32)
  if (MSVC)
    if (USE_DEBUG_CONSOLE)
      set_target_properties(openmw PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE")
      set_target_properties(openmw PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:CONSOLE")
      set_target_properties(openmw PROPERTIES COMPILE_DEFINITIONS_DEBUG "_CONSOLE")
    else()
      # Turn off debug console, debug output will be written to visual studio output instead
      set_target_properties(openmw PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:WINDOWS")
      set_target_properties(openmw PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:WINDOWS")
    endif()

    # Release builds use the debug console
    set_target_properties(openmw PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:CONSOLE")
    set_target_properties(openmw PROPERTIES COMPILE_DEFINITIONS_RELEASE "_CONSOLE")
    set_target_properties(openmw PROPERTIES LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:CONSOLE")
  endif(MSVC)

  # Same for MinGW
  if (MINGW)
    if (USE_DEBUG_CONSOLE)
      set_target_properties(openmw PROPERTIES LINK_FLAGS_DEBUG "-Wl,-subsystem,console")
      set_target_properties(openmw PROPERTIES LINK_FLAGS_RELWITHDEBINFO "-Wl,-subsystem,console")
      set_target_properties(openmw PROPERTIES COMPILE_DEFINITIONS_DEBUG "_CONSOLE")
    else(USE_DEBUG_CONSOLE)
      set_target_properties(openmw PROPERTIES LINK_FLAGS_DEBUG "-Wl,-subsystem,windows")
      set_target_properties(openmw PROPERTIES LINK_FLAGS_RELWITHDEBINFO "-Wl,-subsystem,windows")
    endif(USE_DEBUG_CONSOLE)

    set_target_properties(openmw PROPERTIES LINK_FLAGS_RELEASE "-Wl,-subsystem,console")
    set_target_properties(openmw PROPERTIES LINK_FLAGS_MINSIZEREL "-Wl,-subsystem,console")
    set_target_properties(openmw PROPERTIES COMPILE_DEFINITIONS_RELEASE "_CONSOLE")
  endif(MINGW)

  # TODO: At some point release builds should not use the console but rather write to a log file
  #set_target_properties(openmw PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")
  #set_target_properties(openmw PROPERTIES LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:WINDOWS")
endif()
