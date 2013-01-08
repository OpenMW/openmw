project(OpenMW)

if (APPLE)
    set(APP_BUNDLE_NAME "${CMAKE_PROJECT_NAME}.app")

    set(APP_BUNDLE_DIR "${OpenMW_BINARY_DIR}/${APP_BUNDLE_NAME}")
endif (APPLE)

# Macros

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/)

include (OpenMWMacros)

# Version

set (OPENMW_VERSION_MAJOR 0)
set (OPENMW_VERSION_MINOR 20)
set (OPENMW_VERSION_RELEASE 0)

set (OPENMW_VERSION "${OPENMW_VERSION_MAJOR}.${OPENMW_VERSION_MINOR}.${OPENMW_VERSION_RELEASE}")

# doxygen main page

configure_file ("${OpenMW_SOURCE_DIR}/Docs/mainpage.hpp.cmake" "${OpenMW_SOURCE_DIR}/Docs/mainpage.hpp")

option(MYGUI_STATIC "Link static build of Mygui into the binaries" FALSE)
option(OGRE_STATIC  "Link static build of Ogre and Ogre Plugins into the binaries" FALSE)
option(BOOST_STATIC "Link static build of Boost into the binaries" FALSE)

# Apps and tools
option(BUILD_ESMTOOL "build ESM inspector" ON)
option(BUILD_LAUNCHER "build Launcher" ON)
option(BUILD_MWINIIMPORTER "build MWiniImporter" ON)
option(BUILD_OPENCS "build OpenMW Construction Set" ON)
option(BUILD_WITH_CODE_COVERAGE "Enable code coverage with gconv" OFF)
option(BUILD_UNITTESTS "Enable Unittests with Google C++ Unittest ang GMock frameworks" OFF)

# Sound source selection
option(USE_FFMPEG "use ffmpeg for sound" ON)
option(USE_AUDIERE "use audiere for sound" ON)
option(USE_MPG123 "use mpg123 + libsndfile for sound" ON)

# OS X deployment
option(OPENMW_OSX_DEPLOYMENT OFF)

find_program(DPKG_PROGRAM dpkg DOC "dpkg program of Debian-based systems")

# Location of morrowind data files
if(DPKG_PROGRAM)
    set(MORROWIND_DATA_FILES "/usr/share/games/openmw/data/" CACHE PATH "location of Morrowind data files")
    set(MORROWIND_RESOURCE_FILES "/usr/share/games/openmw/resources/" CACHE PATH "location of OpenMW resources files")
else()
    if (APPLE)
        set(MORROWIND_DATA_FILES "./data" CACHE PATH "location of Morrowind data files")
        set(MORROWIND_RESOURCE_FILES "./resources" CACHE PATH "location of OpenMW resources files")
    else()
        set(MORROWIND_DATA_FILES "data" CACHE PATH "location of Morrowind data files")
        set(MORROWIND_RESOURCE_FILES "resources" CACHE PATH "location of OpenMW resources files")
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

    set(BULLET_ROOT "${PREBUILT_DIR}/bullet")
ELSE()
    message (STATUS "OpenMW pre-built binaries not found.  Using standard locations.")
ENDIF()

# source directory: libs

set(LIBDIR ${CMAKE_SOURCE_DIR}/libs)

set(OENGINE_OGRE
  ${LIBDIR}/openengine/ogre/renderer.cpp
  ${LIBDIR}/openengine/ogre/fader.cpp
  ${LIBDIR}/openengine/ogre/imagerotate.cpp
  ${LIBDIR}/openengine/ogre/selectionbuffer.cpp
)
set(OENGINE_GUI
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
    ${LIBDIR}/openengine/bullet/pmove.cpp
    ${LIBDIR}/openengine/bullet/pmove.h
    ${LIBDIR}/openengine/bullet/trace.cpp
    ${LIBDIR}/openengine/bullet/trace.h

)

set(OENGINE_ALL ${OENGINE_OGRE} ${OENGINE_GUI} ${OENGINE_BULLET})
source_group(libs\\openengine FILES ${OENGINE_ALL})

set(OPENMW_LIBS ${OENGINE_ALL})
set(OPENMW_LIBS_HEADER)

# Sound setup
set(GOT_SOUND_INPUT 0)
set(SOUND_INPUT_INCLUDES "")
set(SOUND_INPUT_LIBRARY "")
set(SOUND_DEFINE "")
if (USE_FFMPEG)
    set(FFmpeg_FIND_COMPONENTS AVCODEC AVFORMAT AVUTIL SWSCALE)
    find_package(FFmpeg)
    if (FFMPEG_FOUND)
        set(SOUND_INPUT_INCLUDES ${SOUND_INPUT_INCLUDES} ${FFMPEG_INCLUDE_DIRS})
        set(SOUND_INPUT_LIBRARY ${SOUND_INPUT_LIBRARY} ${FFMPEG_LIBRARIES} ${SWSCALE_LIBRARIES})
        set(SOUND_DEFINE ${SOUND_DEFINE} -DOPENMW_USE_FFMPEG)
        set(GOT_SOUND_INPUT 1)
    endif (FFMPEG_FOUND)
endif (USE_FFMPEG)

if (USE_AUDIERE AND NOT GOT_SOUND_INPUT)
    find_package(Audiere)
    if (AUDIERE_FOUND)
        set(SOUND_INPUT_INCLUDES ${SOUND_INPUT_INCLUDES} ${AUDIERE_INCLUDE_DIR})
        set(SOUND_INPUT_LIBRARY ${SOUND_INPUT_LIBRARY} ${AUDIERE_LIBRARY})
        set(SOUND_DEFINE ${SOUND_DEFINE} -DOPENMW_USE_AUDIERE)
        set(GOT_SOUND_INPUT 1)
    endif (AUDIERE_FOUND)
endif (USE_AUDIERE AND NOT GOT_SOUND_INPUT)

if (USE_MPG123 AND NOT GOT_SOUND_INPUT)
    find_package(MPG123 REQUIRED)
    find_package(SNDFILE REQUIRED)
    if (MPG123_FOUND AND SNDFILE_FOUND)
        set(SOUND_INPUT_INCLUDES ${SOUND_INPUT_INCLUDES} ${MPG123_INCLUDE_DIR} ${SNDFILE_INCLUDE_DIR})
        set(SOUND_INPUT_LIBRARY ${SOUND_INPUT_LIBRARY} ${MPG123_LIBRARY} ${SNDFILE_LIBRARY})
        set(SOUND_DEFINE ${SOUND_DEFINE} -DOPENMW_USE_MPG123)
        set(GOT_SOUND_INPUT 1)
    endif (MPG123_FOUND AND SNDFILE_FOUND)
endif (USE_MPG123 AND NOT GOT_SOUND_INPUT)

if (NOT GOT_SOUND_INPUT)
    message(WARNING "--------------------")
    message(WARNING "Failed to find any sound input packages")
    message(WARNING "--------------------")
endif (NOT GOT_SOUND_INPUT)

if (NOT FFMPEG_FOUND)
    message(WARNING "--------------------")
    message(WARNING "FFmpeg not found, video playback will be disabled")
    message(WARNING "--------------------")
endif (NOT FFMPEG_FOUND)


# Platform specific
if (WIN32)
    set(Boost_USE_STATIC_LIBS   ON)
    set(PLATFORM_INCLUDE_DIR "platform")
    add_definitions(-DBOOST_ALL_NO_LIB)
else (WIN32)
    set(PLATFORM_INCLUDE_DIR "")
    find_path (UUID_INCLUDE_DIR uuid/uuid.h)
    include_directories(${UUID_INCLUDE_DIR})
endif (WIN32)
if (MSVC10)
    set(PLATFORM_INCLUDE_DIR "")
endif()

if (APPLE)
    set(Boost_USE_STATIC_LIBS   ON)
endif (APPLE)

# Dependencies

# Fix for not visible pthreads functions for linker with glibc 2.15
if (UNIX AND NOT APPLE)
    find_package (Threads)
endif()

# find boost without components so we can use Boost_VERSION
find_package(Boost REQUIRED)

set(BOOST_COMPONENTS system filesystem program_options thread)

if (Boost_VERSION LESS 104900)
    set(SHINY_USE_WAVE_SYSTEM_INSTALL "TRUE")
    set(BOOST_COMPONENTS ${BOOST_COMPONENTS} wave)
endif()

IF(BOOST_STATIC)
    set(Boost_USE_STATIC_LIBS   ON)
endif()

find_package(OGRE REQUIRED)
find_package(MyGUI REQUIRED)
find_package(Boost REQUIRED COMPONENTS ${BOOST_COMPONENTS})
find_package(OIS REQUIRED)
find_package(OpenAL REQUIRED)
find_package(Bullet REQUIRED)
IF(OGRE_STATIC)
find_package(Cg)
IF(WIN32)
set(OGRE_PLUGIN_INCLUDE_DIRS ${OGRE_Plugin_CgProgramManager_INCLUDE_DIRS} ${OGRE_Plugin_OctreeSceneManager_INCLUDE_DIRS} ${OGRE_Plugin_ParticleFX_INCLUDE_DIRS} ${OGRE_RenderSystem_Direct3D9_INCLUDE_DIRS} ${OGRE_RenderSystem_GL_INCLUDE_DIRS})
ELSE(WIN32)
set(OGRE_PLUGIN_INCLUDE_DIRS ${OGRE_Plugin_CgProgramManager_INCLUDE_DIRS} ${OGRE_Plugin_OctreeSceneManager_INCLUDE_DIRS} ${OGRE_Plugin_ParticleFX_INCLUDE_DIRS} ${OGRE_RenderSystem_GL_INCLUDE_DIRS})
ENDIF(WIN32)
ENDIF(OGRE_STATIC)
include_directories("."
    ${OGRE_INCLUDE_DIR} ${OGRE_INCLUDE_DIR}/Ogre ${OGRE_INCLUDE_DIR}/OGRE ${OGRE_PLUGIN_INCLUDE_DIRS}
    ${OGRE_Terrain_INCLUDE_DIR}
    ${OIS_INCLUDE_DIRS} ${Boost_INCLUDE_DIR}
    ${PLATFORM_INCLUDE_DIR}
    ${MYGUI_INCLUDE_DIRS}
    ${MYGUI_PLATFORM_INCLUDE_DIRS}
    ${OPENAL_INCLUDE_DIR}
    ${UUID_INCLUDE_DIR}
    ${LIBDIR}
)

link_directories(${Boost_LIBRARY_DIRS} ${OGRE_LIB_DIR} ${MYGUI_LIB_DIR})

if (APPLE)
    # List used Ogre plugins
    SET(USED_OGRE_PLUGINS ${OGRE_RenderSystem_GL_LIBRARY_REL}
                          ${OGRE_Plugin_OctreeSceneManager_LIBRARY_REL}
                          ${OGRE_Plugin_CgProgramManager_LIBRARY_REL}
                          ${OGRE_Plugin_ParticleFX_LIBRARY_REL})

    if (${OGRE_PLUGIN_DIR_REL}})
        set(OGRE_PLUGINS_REL_FOUND TRUE)
    endif ()

    if (${OGRE_PLUGIN_DIR_DBG})
        set(OGRE_PLUGINS_DBG_FOUND TRUE)
    endif ()

    if (${OGRE_PLUGINS_REL_FOUND})
        set(OGRE_PLUGIN_DIR ${OGRE_PLUGIN_DIR_REL})
    else ()
        set(OGRE_PLUGIN_DIR ${OGRE_PLUGIN_DIR_DBG})
    endif ()

    #set(OGRE_PLUGIN_DIR "${OGRE_PLUGIN_DIR}/")

    configure_file(${OpenMW_SOURCE_DIR}/files/mac/Info.plist
        "${APP_BUNDLE_DIR}/Contents/Info.plist")

    configure_file(${OpenMW_SOURCE_DIR}/files/mac/openmw.icns
        "${APP_BUNDLE_DIR}/Contents/Resources/OpenMW.icns" COPYONLY)
endif (APPLE)

# Set up DEBUG define
set_directory_properties(PROPERTIES COMPILE_DEFINITIONS_DEBUG DEBUG=1)

# Set up Ogre plugin folder & debug suffix
if (APPLE)
	# Ogre on OS X doesn't use "_d" suffix (see Ogre's CMakeLists.txt)
	add_definitions(-DOGRE_PLUGIN_DEBUG_SUFFIX="")
else ()
	add_definitions(-DOGRE_PLUGIN_DEBUG_SUFFIX="_d")
endif()

add_definitions(-DOGRE_PLUGIN_DIR_REL="${OGRE_PLUGIN_DIR_REL}")
add_definitions(-DOGRE_PLUGIN_DIR_DBG="${OGRE_PLUGIN_DIR_DBG}")
if (APPLE AND OPENMW_OSX_DEPLOYMENT)
    add_definitions(-DOGRE_PLUGIN_DIR="${APP_BUNDLE_NAME}/Contents/Plugins")
else()
    add_definitions(-DOGRE_PLUGIN_DIR="${OGRE_PLUGIN_DIR}")
endif()


add_subdirectory(files/)
add_subdirectory(files/mygui)

# Specify build paths

if (APPLE)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${APP_BUNDLE_DIR}/Contents/MacOS")
else (APPLE)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${OpenMW_BINARY_DIR}")
endif (APPLE)

# Other files

configure_file(${OpenMW_SOURCE_DIR}/files/settings-default.cfg
    "${OpenMW_BINARY_DIR}/settings-default.cfg")

configure_file(${OpenMW_SOURCE_DIR}/files/transparency-overrides.cfg
    "${OpenMW_BINARY_DIR}/transparency-overrides.cfg")

configure_file(${OpenMW_SOURCE_DIR}/files/openmw.cfg.local
    "${OpenMW_BINARY_DIR}/openmw.cfg")
configure_file(${OpenMW_SOURCE_DIR}/files/openmw.cfg
    "${OpenMW_BINARY_DIR}/openmw.cfg.install")


if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    configure_file(${OpenMW_SOURCE_DIR}/files/openmw.desktop
        "${OpenMW_BINARY_DIR}/openmw.desktop")
endif()

# Compiler settings
if (CMAKE_COMPILER_IS_GNUCC)
    add_definitions (-Wall -Wextra -Wno-unused-parameter -Wno-reorder -std=c++0x -pedantic)

    # Silence warnings in OGRE headers. Remove once OGRE got fixed!
    add_definitions (-Wno-ignored-qualifiers)

    execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion
                OUTPUT_VARIABLE GCC_VERSION)
    if ("${GCC_VERSION}" VERSION_GREATER 4.6 OR "${GCC_VERSION}" VERSION_EQUAL 4.6)
        add_definitions (-Wno-unused-but-set-parameter)
    endif("${GCC_VERSION}" VERSION_GREATER 4.6 OR "${GCC_VERSION}" VERSION_EQUAL 4.6)
endif (CMAKE_COMPILER_IS_GNUCC)

if(DPKG_PROGRAM)
    SET(CMAKE_INSTALL_PREFIX "/usr")

    if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/.git")
        exec_program("git" ${CMAKE_CURRENT_SOURCE_DIR} ARGS "describe" OUTPUT_VARIABLE GIT_VERSION )
        STRING(REGEX REPLACE "openmw-" "" VERSION_STRING "${GIT_VERSION}")
        exec_program("git" ARGS "config --get user.name" OUTPUT_VARIABLE GIT_NAME )
        exec_program("git" ARGS "config --get user.email" OUTPUT_VARIABLE GIT_EMAIL)
        set(PACKAGE_MAINTAINER "${GIT_NAME} <${GIT_EMAIL}>")
    else()
        set(VERSION_STRING "${OPENMW_VERSION}")
        set(PACKAGE_MAINTAINER "unknown")
    endif()

    #Install icon and desktop file
    INSTALL(FILES "${OpenMW_BINARY_DIR}/openmw.desktop" DESTINATION "share/applications/" PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ COMPONENT "openmw")
    INSTALL(FILES "${OpenMW_SOURCE_DIR}/apps/launcher/resources/images/openmw.png" DESTINATION "share/pixmaps/" PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ COMPONENT "openmw")

    #Install global configuration files
    INSTALL(FILES "${OpenMW_BINARY_DIR}/settings-default.cfg" DESTINATION "../etc/openmw/" PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ COMPONENT "openmw")
    INSTALL(FILES "${OpenMW_BINARY_DIR}/transparency-overrides.cfg" DESTINATION "../etc/openmw/" PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ COMPONENT "openmw")
    INSTALL(FILES "${OpenMW_BINARY_DIR}/openmw.cfg.install" DESTINATION "../etc/openmw/" RENAME "openmw.cfg" PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ COMPONENT "openmw")

    #Install resources
    INSTALL(DIRECTORY "${OpenMW_BINARY_DIR}/resources" DESTINATION "share/games/openmw/" FILE_PERMISSIONS OWNER_READ GROUP_READ WORLD_READ COMPONENT "Resources")
    INSTALL(DIRECTORY DESTINATION "share/games/openmw/data/" COMPONENT "Resources")

    SET(CPACK_GENERATOR "DEB")
    SET(CPACK_PACKAGE_NAME "openmw")
    SET(CPACK_DEBIAN_PACKAGE_HOMEPAGE "http://openmw.org")
    SET(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
    SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "${PACKAGE_MAINTAINER}")
    SET(CPACK_DEBIAN_PACKAGE_DESCRIPTION "A reimplementation of The Elder Scrolls III: Morrowind
 OpenMW is a reimplementation of the Bethesda Game Studios game The Elder Scrolls III: Morrowind.
 Data files from the original game is required to run it.")
    SET(CPACK_DEBIAN_PACKAGE_NAME "openmw")
    SET(CPACK_DEBIAN_PACKAGE_VERSION "${VERSION_STRING}")
    SET(CPACK_PACKAGE_EXECUTABLES "openmw;OpenMW esmtool;Esmtool omwlauncher;OMWLauncher mwiniimporter;MWiniImporter")
    SET(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.11.2), libfreetype6 (>= 2.2.1), libgcc1 (>= 1:4.1.1), libmpg123-0 (>= 1.12.1), libois-1.3.0 (>= 1.3.0), libopenal1 (>= 1:1.12.854), libsndfile1 (>= 1.0.23), libstdc++6 (>= 4.4.5), libuuid1 (>= 2.17.2), libqtgui4 (>= 4.7.0)")

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

if(WIN32)
    FILE(GLOB dll_files "${OpenMW_BINARY_DIR}/Release/*.dll")
    INSTALL(FILES ${dll_files} DESTINATION ".")
    INSTALL(FILES "${OpenMW_BINARY_DIR}/openmw.cfg.install" DESTINATION "." RENAME "openmw.cfg")
    INSTALL(FILES
        "${OpenMW_SOURCE_DIR}/readme.txt"
        "${OpenMW_SOURCE_DIR}/GPL3.txt"
        "${OpenMW_SOURCE_DIR}/OFL.txt"
        "${OpenMW_SOURCE_DIR}/Bitstream Vera License.txt"
        "${OpenMW_SOURCE_DIR}/Daedric Font License.txt"
        "${OpenMW_BINARY_DIR}/launcher.qss"
        "${OpenMW_BINARY_DIR}/settings-default.cfg"
        "${OpenMW_BINARY_DIR}/transparency-overrides.cfg"
        "${OpenMW_BINARY_DIR}/Release/omwlauncher.exe"
        "${OpenMW_BINARY_DIR}/Release/openmw.exe"
        DESTINATION ".")
    INSTALL(DIRECTORY "${OpenMW_BINARY_DIR}/resources" DESTINATION ".")

    SET(CPACK_GENERATOR "NSIS")
    SET(CPACK_PACKAGE_NAME "OpenMW")
    SET(CPACK_PACKAGE_VENDOR "OpenMW.org")
    SET(CPACK_PACKAGE_VERSION ${OPENMW_VERSION})
    SET(CPACK_PACKAGE_VERSION_MAJOR ${OPENMW_VERSION_MAJOR})
    SET(CPACK_PACKAGE_VERSION_MINOR ${OPENMW_VERSION_MINO})
    SET(CPACK_PACKAGE_VERSION_PATCH ${OPENMW_VERSION_RELEASE})
    SET(CPACK_PACKAGE_EXECUTABLES "openmw;OpenMW;omwlauncher;OpenMW Launcher")
    SET(CPACK_NSIS_CREATE_ICONS_EXTRA "CreateShortCut '\$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\Readme.lnk' '\$INSTDIR\\\\readme.txt'")
    SET(CPACK_NSIS_DELETE_ICONS_EXTRA "
        !insertmacro MUI_STARTMENU_GETFOLDER Application $MUI_TEMP
        Delete \\\"$SMPROGRAMS\\\\$MUI_TEMP\\\\Readme.lnk\\\"
        ")
    SET(CPACK_RESOURCE_FILE_README "${OpenMW_SOURCE_DIR}/readme.txt")
    SET(CPACK_PACKAGE_DESCRIPTION_FILE "${OpenMW_SOURCE_DIR}/readme.txt")
    SET(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")
    SET(CPACK_NSIS_DISPLAY_NAME "OpenMW")
    SET(CPACK_NSIS_HELP_LINK "http:\\\\\\\\www.openmw.org")
    SET(CPACK_NSIS_URL_INFO_ABOUT "http:\\\\\\\\www.openmw.org")
    SET(CPACK_NSIS_INSTALLED_ICON_NAME "omwlauncher.exe")
    SET(CPACK_NSIS_MUI_ICON "${OpenMW_SOURCE_DIR}/apps/launcher/resources/images/openmw.ico")
    SET(CPACK_NSIS_MUI_UNIICON "${OpenMW_SOURCE_DIR}/apps/launcher/resources/images/openmw.ico")
    SET(CPACK_PACKAGE_ICON "${OpenMW_SOURCE_DIR}\\\\files\\\\openmw.bmp")

    SET(VCREDIST32 "${OpenMW_BINARY_DIR}/vcredist_x86.exe")
    if(EXISTS ${VCREDIST32})
        INSTALL(FILES ${VCREDIST32} DESTINATION "redist")
        SET(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "ExecWait '\\\"$INSTDIR\\\\redist\\\\vcredist_x86.exe\\\" /q'" )
    endif(EXISTS ${VCREDIST32})

    SET(VCREDIST64 "${OpenMW_BINARY_DIR}/vcredist_x64.exe")
    if(EXISTS ${VCREDIST64})
        INSTALL(FILES ${VCREDIST64} DESTINATION "redist")
        SET(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "ExecWait '\\\"$INSTDIR\\\\redist\\\\vcredist_x64.exe\\\" /q'" )
    endif(EXISTS ${VCREDIST64})

    SET(OALREDIST "${OpenMW_BINARY_DIR}/oalinst.exe")
    if(EXISTS ${OALREDIST})
        INSTALL(FILES ${OALREDIST} DESTINATION "redist")
        SET(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "${CPACK_NSIS_EXTRA_INSTALL_COMMANDS}
            ExecWait '\\\"$INSTDIR\\\\redist\\\\oalinst.exe\\\" /s'" )
    endif(EXISTS ${OALREDIST})

    if(CMAKE_CL_64)
        SET(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
    endif()

    include(CPack)
endif(WIN32)

# Extern
add_subdirectory (extern/shiny)
add_subdirectory (extern/oics)

# Components
add_subdirectory (components)

# Apps and tools
add_subdirectory( apps/openmw )

if (BUILD_ESMTOOL)
  add_subdirectory( apps/esmtool )
endif()

if (BUILD_LAUNCHER)
   add_subdirectory( apps/launcher )
endif()

if (BUILD_MWINIIMPORTER)
   add_subdirectory( apps/mwiniimporter )
endif()

if (BUILD_OPENCS)
   add_subdirectory (apps/opencs)
endif()

# UnitTests
if (BUILD_UNITTESTS)
  add_subdirectory( apps/openmw_test_suite )
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

    # Play a bit with the warning levels

    set(WARNINGS "/Wall") # Since windows can only disable specific warnings, not enable them

    set(WARNINGS_DISABLE
        # Warnings that aren't enabled normally and don't need to be enabled
        # They're unneeded and sometimes completely retarded warnings that /Wall enables
        # Not going to bother commenting them as they tend to warn on every standard library files
        4061 4263 4264 4266 4350 4514 4548 4571 4610 4619 4623 4625 4626 4628 4640 4668 4710 4711 4820 4826 4917 4946

        # Warnings that are thrown on standard libraries and not OpenMW
        4347 # Non-template function with same name and parameter count as template function
        4365 # Variable signed/unsigned mismatch
        4510 4512 # Unable to generate copy constructor/assignment operator as it's not public in the base
        4706 # Assignment in conditional expression
        4738 # Storing 32-bit float result in memory, possible loss of performance
        4986 # Undocumented warning that occurs in the crtdbg.h file
        4996 # Function was declared deprecated

        # OpenMW specific warnings
        4099 # Type mismatch, declared class or struct is defined with other type
        4100 # Unreferenced formal parameter (-Wunused-parameter)
        4127 # Conditional expression is constant
        4242 # Storing value in a variable of a smaller type, possible loss of data
        4244 # Storing value of one type in variable of another (size_t in int, for example)
        4305 # Truncating value (double to float, for example)
        4309 # Variable overflow, trying to store 128 in a signed char for example
        4355 # Using 'this' in member initialization list
        4701 # Potentially uninitialized local variable used
		4800 # Boolean optimization warning, e.g. myBool = (myInt != 0) instead of myBool = myInt
        )

    foreach(d ${WARNINGS_DISABLE})
        set(WARNINGS "${WARNINGS} /wd${d}")
    endforeach(d)

    set_target_properties(components PROPERTIES COMPILE_FLAGS ${WARNINGS})
    if (BUILD_LAUNCHER)
		set_target_properties(omwlauncher PROPERTIES COMPILE_FLAGS ${WARNINGS})
	endif (BUILD_LAUNCHER)
    set_target_properties(openmw PROPERTIES COMPILE_FLAGS ${WARNINGS})
    if (BUILD_ESMTOOL)
        set_target_properties(esmtool PROPERTIES COMPILE_FLAGS ${WARNINGS})
    endif (BUILD_ESMTOOL)
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

# Apple bundling
if (APPLE)
    set(INSTALL_SUBDIR OpenMW)

    install(DIRECTORY "${APP_BUNDLE_DIR}" USE_SOURCE_PERMISSIONS DESTINATION "${INSTALL_SUBDIR}" COMPONENT Runtime)
    install(DIRECTORY "${OpenMW_BINARY_DIR}/resources" DESTINATION "${INSTALL_SUBDIR}" COMPONENT Runtime)
    install(FILES "${OpenMW_BINARY_DIR}/openmw.cfg.install" RENAME "openmw.cfg" DESTINATION "${INSTALL_SUBDIR}" COMPONENT Runtime)
    install(FILES "${OpenMW_BINARY_DIR}/launcher.qss" DESTINATION "${INSTALL_SUBDIR}" COMPONENT Runtime)

    install(FILES "${OpenMW_BINARY_DIR}/settings-default.cfg" DESTINATION "${INSTALL_SUBDIR}" COMPONENT Runtime)
    install(FILES "${OpenMW_BINARY_DIR}/transparency-overrides.cfg" DESTINATION "${INSTALL_SUBDIR}" COMPONENT Runtime)

    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_PACKAGE_VERSION ${OPENMW_VERSION})
    set(CPACK_PACKAGE_VERSION_MAJOR ${OPENMW_VERSION_MAJOR})
    set(CPACK_PACKAGE_VERSION_MINOR ${OPENMW_VERSION_MINO})
    set(CPACK_PACKAGE_VERSION_PATCH ${OPENMW_VERSION_RELEASE})

    set(APPS "\${CMAKE_INSTALL_PREFIX}/${INSTALL_SUBDIR}/${APP_BUNDLE_NAME}")
    set(PLUGINS "")
    set(ABSOLUTE_PLUGINS "")

    foreach (PLUGIN ${USED_OGRE_PLUGINS})
        get_filename_component(PLUGIN_ABS ${PLUGIN} REALPATH)
        set(ABSOLUTE_PLUGINS ${PLUGIN_ABS} ${ABSOLUTE_PLUGINS})
    endforeach ()

    set(PLUGIN_INSTALL_BASE "\${CMAKE_INSTALL_PREFIX}/${INSTALL_SUBDIR}/${APP_BUNDLE_NAME}/Contents/Plugins")
    install(FILES ${ABSOLUTE_PLUGINS} DESTINATION "${INSTALL_SUBDIR}/${APP_BUNDLE_NAME}/Contents/Plugins" COMPONENT Runtime)
    foreach (PLUGIN ${ABSOLUTE_PLUGINS})
        get_filename_component(PLUGIN_RELATIVE ${PLUGIN} NAME)
        set(PLUGINS ${PLUGINS} "${PLUGIN_INSTALL_BASE}/${PLUGIN_RELATIVE}")
    endforeach ()

    #For now, search unresolved dependencies only in default system paths, so if you put unresolveable (i.e. with @executable_path in id name) lib or framework somewhere else, it would fail
    set(DIRS "")

    # Overriding item resolving during installation, it needed if
    # some library already has been "fixed up", i.e. its id name contains @executable_path,
    # but library is not embedded in bundle. For example, it's Ogre.framework from Ogre SDK.
    # Current implementation of GetPrerequsities/BundleUtilities doesn't handle that case.
    #
    # Current limitations:
    #   1. Handles only frameworks, not simple libs
    INSTALL(CODE "
        set(CMAKE_FIND_LIBRARY_PREFIXES ${CMAKE_FIND_LIBRARY_PREFIXES})
        set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
        set(CMAKE_SYSTEM_FRAMEWORK_PATH ${CMAKE_SYSTEM_FRAMEWORK_PATH})

        set(OPENMW_RESOLVED_ITEMS \"\")

        function(gp_resolve_item_override context item exepath dirs resolved_item_var resolved_var)
            if(item MATCHES \"@executable_path\" AND NOT \${\${resolved_var}})
                if (item MATCHES \"Frameworks\") # if it is a framework
                    # get last segment of path
                    get_filename_component(fname \"\${item}\" NAME_WE)
                    find_library(ri NAMES \${fname} PATHS \${exepath} \${dirs} \${CMAKE_SYSTEM_FRAMEWORK_PATH})
                    if (ri)
                        string(REGEX REPLACE \"^.*/Frameworks/.*\\\\.framework\" \"\" item_part \${item})
                        set(ri \"\${ri}\${item_part}\")
                        set(\${resolved_item_var} \${ri} PARENT_SCOPE)
                        set(\${resolved_var} 1 PARENT_SCOPE)
                    endif()
                else()
                    # code path for standard (non-framework) libs (ogre & qt pugins)
                    get_filename_component(fname \"\${item}\" NAME_WE)
                    string(REGEX REPLACE \"^lib\" \"\" fname \${fname})
                    find_library(ri NAMES \${fname} PATHS \${exepath} \${dirs} /usr/lib /usr/local/lib)
                    if (ri)
                        set(\${resolved_item_var} \${ri} PARENT_SCOPE)
                        set(\${resolved_var} 1 PARENT_SCOPE)
                    endif ()
                endif()
            endif()
        endfunction(gp_resolve_item_override)

        cmake_policy(SET CMP0009 OLD)
        set(BU_CHMOD_BUNDLE_ITEMS ON)
        include(BundleUtilities)
        fixup_bundle(\"${APPS}\" \"${PLUGINS}\" \"${DIRS}\")
        " COMPONENT Runtime)
        include(CPack)
endif (APPLE)

if (NOT WIN32 AND NOT DPKG_PROGRAM AND NOT APPLE)
    ## Non Debian based Linux building
    # paths
    set(BINDIR "${CMAKE_INSTALL_PREFIX}/usr/bin" CACHE PATH "Where to install binaries")
    set(DATAROOTDIR "${CMAKE_INSTALL_PREFIX}/share" CACHE PATH "Sets the root of data directories to a non-default location")
    set(DATADIR "${DATAROOTDIR}/openmw" CACHE PATH "Sets the openmw data directories to a non-default location")
    set(DOCDIR "${DATAROOTDIR}/doc/openmw" CACHE PATH "Sets the doc directory to a non-default location.")
    set(MANDIR "${DATAROOTDIR}/man" CACHE PATH "Where to install manpages")
    set(SYSCONFDIR "${CMAKE_INSTALL_PREFIX}/etc/openmw" CACHE PATH "Set config dir")
    set(ICONDIR "${DATAROOTDIR}/pixmaps" CACHE PATH "Set icon dir")

    # Install binaries
    INSTALL(PROGRAMS "${OpenMW_BINARY_DIR}/openmw" DESTINATION "${BINDIR}" )
    INSTALL(PROGRAMS "${OpenMW_BINARY_DIR}/omwlauncher" DESTINATION "${BINDIR}" )
    INSTALL(PROGRAMS "${OpenMW_BINARY_DIR}/esmtool" DESTINATION "${BINDIR}" )
    INSTALL(PROGRAMS "${OpenMW_BINARY_DIR}/mwiniimport" DESTINATION "${BINDIR}" )

    # Install icon and .desktop
    INSTALL(FILES "${OpenMW_SOURCE_DIR}/apps/launcher/resources/images/openmw.png" DESTINATION "${ICONDIR}")
    INSTALL(FILES "${OpenMW_BINARY_DIR}/openmw.desktop" DESTINATION "${DATAROOTDIR}/applications")

    # Install global configuration files
    INSTALL(FILES "${OpenMW_BINARY_DIR}/openmw.cfg.install" DESTINATION "${SYSCONFDIR}" RENAME "openmw.cfg" )
    #INSTALL(FILES "${OpenMW_BINARY_DIR}/plugins.cfg" DESTINATION "${SYSCONFDIR}" )
    INSTALL(FILES "${OpenMW_BINARY_DIR}/settings-default.cfg" DESTINATION "${SYSCONFDIR}" )
    INSTALL(FILES "${OpenMW_BINARY_DIR}/transparency-overrides.cfg" DESTINATION "${SYSCONFDIR}" )

    # Install resources
    INSTALL(DIRECTORY "${OpenMW_BINARY_DIR}/resources" DESTINATION "${DATADIR}" )
    INSTALL(FILES "${OpenMW_BINARY_DIR}/launcher.qss" DESTINATION "${DATADIR}/resources" )
endif(NOT WIN32 AND NOT DPKG_PROGRAM AND NOT APPLE)
