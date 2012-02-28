project(OpenMW)

if (APPLE)
    set(APP_BUNDLE_NAME "${CMAKE_PROJECT_NAME}.app")

    set(APP_BUNDLE_DIR "${OpenMW_BINARY_DIR}/${APP_BUNDLE_NAME}")

    # using 10.6 sdk
    set(CMAKE_OSX_SYSROOT "/Developer/SDKs/MacOSX10.6.sdk")
endif (APPLE)

# Macros

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/)

include (OpenMWMacros)

# Version

set (OPENMW_VERSION_MAJOR 0)
set (OPENMW_VERSION_MINOR 12)
set (OPENMW_VERSION_RELEASE 0)

set (OPENMW_VERSION "${OPENMW_VERSION_MAJOR}.${OPENMW_VERSION_MINOR}.${OPENMW_VERSION_RELEASE}")

# doxygen main page

configure_file ("${OpenMW_SOURCE_DIR}/Docs/mainpage.hpp.cmake" "${OpenMW_SOURCE_DIR}/Docs/mainpage.hpp")

# Sound source selection
option(USE_AUDIERE "use Audiere for sound" OFF)
option(USE_FFMPEG "use ffmpeg for sound" OFF)
option(USE_MPG123 "use mpg123 + libsndfile for sound" ON)

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
    ${OIS_INCLUDE_DIRS} ${Boost_INCLUDE_DIR}
    ${PLATFORM_INCLUDE_DIR}
    ${CMAKE_HOME_DIRECTORY}/extern/caelum/include
    ${CMAKE_HOME_DIRECTORY}/extern/mygui_3.0.1/MyGUIEngine/include
    ${CMAKE_HOME_DIRECTORY}/extern/mygui_3.0.1/OgrePlatform/include
    ${OPENAL_INCLUDE_DIR}
    ${UUID_INCLUDE_DIR}
    ${LIBDIR}
)

link_directories(${Boost_LIBRARY_DIRS} ${OGRE_LIB_DIR})

if(APPLE)
    # List used Ogre plugins
    SET(USED_OGRE_PLUGINS "RenderSystem_GL"
                          "Plugin_OctreeSceneManager"
                          "Plugin_ParticleFX")
endif(APPLE)

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

configure_file(${OpenMW_SOURCE_DIR}/files/openmw.cfg.local
    "${OpenMW_BINARY_DIR}/openmw.cfg")
configure_file(${OpenMW_SOURCE_DIR}/files/openmw.cfg
    "${OpenMW_BINARY_DIR}/openmw.cfg.install")

if (WIN32)
    configure_file(${OpenMW_SOURCE_DIR}/files/plugins.cfg.win32
        "${OpenMW_BINARY_DIR}/plugins.cfg" COPYONLY)
endif (WIN32)

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    configure_file(${OpenMW_SOURCE_DIR}/files/plugins.cfg.linux
        "${OpenMW_BINARY_DIR}/plugins.cfg")
endif()

if (APPLE)
    configure_file(${OpenMW_SOURCE_DIR}/files/plugins.cfg.mac
        "${OpenMW_BINARY_DIR}/plugins.cfg")

    configure_file(${OpenMW_SOURCE_DIR}/files/mac/Info.plist
        "${APP_BUNDLE_DIR}/Contents/Info.plist")

    configure_file(${OpenMW_SOURCE_DIR}/files/mac/openmw.icns
        "${APP_BUNDLE_DIR}/Contents/Resources/OpenMW.icns" COPYONLY)

    # prepare plugins
    foreach(plugin ${USED_OGRE_PLUGINS})
        configure_file("${OGRE_PLUGIN_DIR}/${plugin}.dylib"
                        "${APP_BUNDLE_DIR}/Contents/Plugins/${plugin}.dylib"
                        COPYONLY)
    endforeach()
endif (APPLE)


# Compiler settings
if (CMAKE_COMPILER_IS_GNUCC)
    #add_definitions (-Wall -Werror)
    add_definitions (-Wall)
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
    INSTALL(FILES "${OpenMW_SOURCE_DIR}/files/openmw.desktop" DESTINATION "share/applications/" PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ COMPONENT "openmw")
    INSTALL(FILES "${OpenMW_SOURCE_DIR}/apps/launcher/resources/images/openmw.png" DESTINATION "share/pixmaps/" PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ COMPONENT "openmw")

    #Install global configuration files
    INSTALL(FILES "${OpenMW_BINARY_DIR}/openmw.cfg.install" DESTINATION "../etc/openmw/" RENAME "openmw.cfg" PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ COMPONENT "openmw")
    INSTALL(FILES "${OpenMW_BINARY_DIR}/plugins.cfg" DESTINATION "../etc/openmw/" PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ COMPONENT "openmw")

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
    SET(CPACK_PACKAGE_EXECUTABLES "openmw;OpenMW esmtool;Esmtool omwlauncher;OMWLauncher")
    SET(CPACK_DEBIAN_PACKAGE_DEPENDS "libogre-1.7.3 (>= 1.7.3), libbullet0 (>= 2.77),  libboost-filesystem1.46.1 (>= 1.46.1), libboost-program-options1.46.1 (>= 1.46.1), libboost-system1.46.1 (>= 1.46.1), libboost-thread1.46.1 (>= 1.46.1), libc6 (>= 2.11.2), libfreetype6 (>= 2.2.1), libgcc1 (>= 1:4.1.1), libmpg123-0 (>= 1.12.1), libois-1.3.0 (>= 1.3.0), libopenal1 (>= 1:1.12.854), libsndfile1 (>= 1.0.23), libstdc++6 (>= 4.4.5), libuuid1 (>= 2.17.2), libqtgui4 (>= 4.7.0)")

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
    FILE(GLOB files "${OpenMW_BINARY_DIR}/Release/*.*")
    INSTALL(FILES ${files} DESTINATION ".")
    INSTALL(FILES "${OpenMW_BINARY_DIR}/openmw.cfg.install" DESTINATION "." RENAME "openmw.cfg")
    INSTALL(DIRECTORY "${OpenMW_BINARY_DIR}/resources" DESTINATION ".")

    SET(CPACK_GENERATOR "NSIS")
    SET(CPACK_PACKAGE_NAME "OpenMW")
    SET(CPACK_PACKAGE_VENDOR "OpenMW.org")
    SET(CPACK_PACKAGE_VERSION ${OPENMW_VERSION})
    SET(CPACK_PACKAGE_VERSION_MAJOR ${OPENMW_VERSION_MAJOR})
    SET(CPACK_PACKAGE_VERSION_MINOR ${OPENMW_VERSION_MINO})
    SET(CPACK_PACKAGE_VERSION_PATCH ${OPENMW_VERSION_RELEASE})
    SET(CPACK_PACKAGE_EXECUTABLES "openmw;OpenMW;esmtool;Esmtool;omwlauncher;OpenMW Launcher")
    SET(CPACK_PACKAGE_DESCRIPTION_FILE "${OpenMW_SOURCE_DIR}/readme.txt")
    SET(CPACK_RESOURCE_FILE_LICENSE "${OpenMW_SOURCE_DIR}/GPL3.txt")
    SET(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")
    SET(CPACK_NSIS_DISPLAY_NAME "OpenMW")
    SET(CPACK_NSIS_HELP_LINK "http:\\\\\\\\www.openmw.org")
    SET(CPACK_NSIS_URL_INFO_ABOUT "http:\\\\\\\\www.openmw.org")
    SET(CPACK_NSIS_INSTALLED_ICON_NAME "omwlauncher.exe")

    SET(VCREDIST "${OpenMW_BINARY_DIR}/vcredist_x86.exe")
    if(EXISTS ${VCREDIST})
        INSTALL(FILES ${VCREDIST} DESTINATION "redist")
        SET(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "ExecWait '\\\"$INSTDIR\\\\redist\\\\vcredist_x86.exe\\\" /q'" )
    endif(EXISTS ${VCREDIST})

    SET(OALREDIST "${OpenMW_BINARY_DIR}/oalinst.exe")
    if(EXISTS ${OALREDIST})
        INSTALL(FILES ${OALREDIST} DESTINATION "redist")
        SET(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "${CPACK_NSIS_EXTRA_INSTALL_COMMANDS}
            ExecWait '\\\"$INSTDIR\\\\redist\\\\oalinst.exe\\\" /s'" )
    endif(EXISTS ${OALREDIST})

    include(CPack)
endif(WIN32)

# Components
add_subdirectory (components)

# Apps and tools
add_subdirectory( apps/openmw )

option(BUILD_ESMTOOL "build ESM inspector" ON)
if (BUILD_ESMTOOL)
  add_subdirectory( apps/esmtool )
endif()

option(BUILD_LAUNCHER "build Launcher inspector" ON)
if (BUILD_LAUNCHER)
   add_subdirectory( apps/launcher )
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

# Apple bundling
if (APPLE)
    set(INSTALL_SUBDIR OpenMW)

    install(DIRECTORY "${APP_BUNDLE_DIR}" USE_SOURCE_PERMISSIONS DESTINATION "${INSTALL_SUBDIR}" COMPONENT Runtime)
    install(DIRECTORY "${OpenMW_BINARY_DIR}/resources" DESTINATION "${INSTALL_SUBDIR}" COMPONENT Runtime)
    install(FILES "${OpenMW_BINARY_DIR}/openmw.cfg.install" RENAME "openmw.cfg" DESTINATION "${INSTALL_SUBDIR}" COMPONENT Runtime)

    install(FILES "${OpenMW_BINARY_DIR}/plugins.cfg" DESTINATION "${INSTALL_SUBDIR}" COMPONENT Runtime)
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_PACKAGE_VERSION ${OPENMW_VERSION})
    set(CPACK_PACKAGE_VERSION_MAJOR ${OPENMW_VERSION_MAJOR})
    set(CPACK_PACKAGE_VERSION_MINOR ${OPENMW_VERSION_MINO})
    set(CPACK_PACKAGE_VERSION_PATCH ${OPENMW_VERSION_RELEASE})

    set(APPS "\${CMAKE_INSTALL_PREFIX}/${INSTALL_SUBDIR}/${APP_BUNDLE_NAME}")
    set(PLUGINS "")

    # Scan Plugins dir for *.dylibs
    set(PLUGIN_SEARCH_ROOT "${APP_BUNDLE_DIR}/Contents/Plugins")
    file(GLOB_RECURSE ALL_PLUGINS "${PLUGIN_SEARCH_ROOT}/*.dylib")

    set(PLUGIN_INSTALL_BASE "\${CMAKE_INSTALL_PREFIX}/${INSTALL_SUBDIR}/${APP_BUNDLE_NAME}/Contents/Plugins")
    foreach(PLUGIN ${ALL_PLUGINS})
        string(REPLACE "${PLUGIN_SEARCH_ROOT}/" "" PLUGIN_RELATIVE "${PLUGIN}")
        set(PLUGINS ${PLUGINS} "${PLUGIN_INSTALL_BASE}/${PLUGIN_RELATIVE}")
    endforeach()

    #For now, search unresolved dependencies only in default system paths, so if you put unresolveable (i.e. with @executable_path in id name) lib or framework somewhere else, it would fail
    set(DIRS "")

    # Overriding item resolving during installation, it needed if
    # some library already has be "fixed up", i.e. its id name contains @executable_path,
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
                    find_library(ri NAMES \${fname} PATHS \${exepath} \${dirs} /Library/Frameworks)
                    if (ri)
                        message(STATUS \"found \${ri} for \${item}\")
                        string(REGEX REPLACE \"^.*/Frameworks/.*\\\\.framework\" \"\" item_part \${item})
                        set(ri \"\${ri}\${item_part}\")
                        set(\${resolved_item_var} \${ri} PARENT_SCOPE)
                        set(\${resolved_var} 1 PARENT_SCOPE)
                        set(OPENMW_RESOLVED_ITEMS \${_OPENMW_RESOLVED_ITEMS} \${ri})
                    endif()
                else()
                    # code path for standard (non-framework) libs (ogre & qt pugins)
                endif()
            endif()
        endfunction(gp_resolve_item_override)

        cmake_policy(SET CMP0009 OLD)
        set(BU_CHMOD_BUNDLE_ITEMS ON)
        include(BundleUtilities)
        fixup_bundle(\"${APPS}\" \"${PLUGINS}\" \"${DIRS}\")
        " COMPONENT Runtime)

include(CPack)

set(CMAKE_EXE_LINKER_FLAGS "-arch i386")
set(CMAKE_CXX_FLAGS "-arch i386")

endif (APPLE)
