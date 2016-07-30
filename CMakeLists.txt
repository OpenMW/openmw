project(OpenMW)

# If the user doesn't supply a CMAKE_BUILD_TYPE via command line, choose one for them.
IF(NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
        "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel."
        FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS None Debug Release RelWithDebInfo MinSizeRel)
ENDIF()

if (APPLE)
    set(APP_BUNDLE_NAME "${CMAKE_PROJECT_NAME}.app")

    set(APP_BUNDLE_DIR "${OpenMW_BINARY_DIR}/${APP_BUNDLE_NAME}")
endif (APPLE)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/)

if (ANDROID)
    set(CMAKE_FIND_ROOT_PATH ${OPENMW_DEPENDENCIES_DIR} "${CMAKE_FIND_ROOT_PATH}")
    set (OSG_PLUGINS_DIR CACHE STRING "")
endif()

# Version
message(STATUS "Configuring OpenMW...")

set(OPENMW_VERSION_MAJOR 0)
set(OPENMW_VERSION_MINOR 39)
set(OPENMW_VERSION_RELEASE 0)

set(OPENMW_VERSION_COMMITHASH "")
set(OPENMW_VERSION_TAGHASH "")

set(OPENMW_VERSION "${OPENMW_VERSION_MAJOR}.${OPENMW_VERSION_MINOR}.${OPENMW_VERSION_RELEASE}")

set(GIT_CHECKOUT FALSE)
if(EXISTS ${PROJECT_SOURCE_DIR}/.git)
    if(NOT EXISTS ${PROJECT_SOURCE_DIR}/.git/shallow)
        find_package(Git)

        if(GIT_FOUND)
            set(GIT_CHECKOUT TRUE)
        else(GIT_FOUND)
            message(WARNING "Git executable not found")
        endif(GIT_FOUND)
    else(NOT EXISTS ${PROJECT_SOURCE_DIR}/.git/shallow)
        message(STATUS "Shallow Git clone detected, not attempting to retrieve version info")
    endif(NOT EXISTS ${PROJECT_SOURCE_DIR}/.git/shallow)
endif(EXISTS ${PROJECT_SOURCE_DIR}/.git)

# Macros
include(OpenMWMacros)

# doxygen main page

configure_file ("${OpenMW_SOURCE_DIR}/docs/mainpage.hpp.cmake" "${OpenMW_BINARY_DIR}/docs/mainpage.hpp")

option(MYGUI_STATIC "Link static build of Mygui into the binaries" FALSE)
option(BOOST_STATIC "Link static build of Boost into the binaries" FALSE)
option(SDL2_STATIC "Link static build of SDL into the binaries" FALSE)
option(OSG_STATIC "Link static build of OpenSceneGraph into the binaries" FALSE)
option(QT_STATIC "Link static build of QT into the binaries" FALSE)

option(OPENMW_UNITY_BUILD "Use fewer compilation units to speed up compile time" FALSE)

# Apps and tools
option(BUILD_OPENMW "build OpenMW" ON)
option(BUILD_BSATOOL "build BSA extractor" ON)
option(BUILD_ESMTOOL "build ESM inspector" ON)
option(BUILD_LAUNCHER "build Launcher" ON)
option(BUILD_MWINIIMPORTER "build MWiniImporter" ON)
option(BUILD_ESSIMPORTER "build ESS (Morrowind save game) importer" ON)
option(BUILD_OPENCS "build OpenMW Construction Set" ON)
option(BUILD_WIZARD "build Installation Wizard" ON)
option(BUILD_WITH_CODE_COVERAGE "Enable code coverage with gconv" OFF)
option(BUILD_UNITTESTS "Enable Unittests with Google C++ Unittest" OFF)
option(BUILD_NIFTEST "build nif file tester" OFF)
option(BUILD_MYGUI_PLUGIN "build MyGUI plugin for OpenMW resources, to use with MyGUI tools" ON)
option(BUILD_DOCS        "build documentation." OFF )

# what is necessary to build documentation
IF( BUILD_DOCS )
    # Builds the documentation.
    FIND_PACKAGE( Sphinx REQUIRED )
    FIND_PACKAGE( Doxygen REQUIRED )
ENDIF()

# OS X deployment
option(OPENMW_OSX_DEPLOYMENT OFF)

if (MSVC)
    option(OPENMW_MP_BUILD "Build OpenMW with /MP flag" OFF)
    option(OPENMW_LTO_BUILD "Build OpenMW with Link-Time Optimization (Needs ~2GB of RAM)" OFF)
endif()

# Set up common paths
if (APPLE)
    set(MORROWIND_DATA_FILES "./data" CACHE PATH "location of Morrowind data files")
    set(OPENMW_RESOURCE_FILES "./resources" CACHE PATH "location of OpenMW resources files")
elseif(UNIX)
    # Paths
    SET(BINDIR "${CMAKE_INSTALL_PREFIX}/bin" CACHE PATH "Where to install binaries")
    SET(LIBDIR "${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}" CACHE PATH "Where to install libraries")
    SET(DATAROOTDIR "${CMAKE_INSTALL_PREFIX}/share" CACHE PATH "Sets the root of data directories to a non-default location")
    SET(GLOBAL_DATA_PATH "${DATAROOTDIR}/games/" CACHE PATH "Set data path prefix")
    SET(DATADIR "${GLOBAL_DATA_PATH}/openmw" CACHE PATH "Sets the openmw data directories to a non-default location")
    SET(ICONDIR "${DATAROOTDIR}/pixmaps" CACHE PATH "Set icon dir")
    SET(LICDIR "${DATAROOTDIR}/licenses/openmw" CACHE PATH "Sets the openmw license directory to a non-default location.")
    IF("${CMAKE_INSTALL_PREFIX}" STREQUAL "/usr")
        SET(GLOBAL_CONFIG_PATH "/etc/" CACHE PATH "Set config dir prefix")
    ELSE()
        SET(GLOBAL_CONFIG_PATH "${CMAKE_INSTALL_PREFIX}/etc/" CACHE PATH "Set config dir prefix")
    ENDIF()
    SET(SYSCONFDIR "${GLOBAL_CONFIG_PATH}/openmw" CACHE PATH "Set config dir")

    set(MORROWIND_DATA_FILES "${DATADIR}/data" CACHE PATH "location of Morrowind data files")
    set(OPENMW_RESOURCE_FILES "${DATADIR}/resources" CACHE PATH "location of OpenMW resources files")
else()
    set(MORROWIND_DATA_FILES "data" CACHE PATH "location of Morrowind data files")
    set(OPENMW_RESOURCE_FILES "resources" CACHE PATH "location of OpenMW resources files")
endif(APPLE)

if (WIN32)
    option(USE_DEBUG_CONSOLE "whether a debug console should be enabled for debug builds, if false debug output is redirected to Visual Studio output" ON)
endif()

if (NOT BUILD_LAUNCHER AND NOT BUILD_OPENCS AND NOT BUILD_WIZARD)
   set(USE_QT FALSE)
else()
   set(USE_QT TRUE)
endif()

# Dependencies
if (USE_QT)
    set(DESIRED_QT_VERSION 4 CACHE STRING "The QT version OpenMW should use (4 or 5)")
    set_property(CACHE DESIRED_QT_VERSION PROPERTY STRINGS 4 5)
    message(STATUS "Using Qt${DESIRED_QT_VERSION}")

    if (DESIRED_QT_VERSION MATCHES 4)
        find_package(Qt4 REQUIRED COMPONENTS QtCore QtGui QtNetwork QtOpenGL)
    else()
        find_package(Qt5Widgets REQUIRED)
        find_package(Qt5Core REQUIRED)
        find_package(Qt5Network REQUIRED)
        find_package(Qt5OpenGL REQUIRED)
    # Instruct CMake to run moc automatically when needed.
    #set(CMAKE_AUTOMOC ON)
    endif()
endif()

if (APPLE)
    # OS X build process relies on this fix: https://github.com/Kitware/CMake/commit/3df5147043d83aa09acd5c9ce31d5c602efb99db
    cmake_minimum_required(VERSION 3.1.0)
elseif (USE_QT AND DESIRED_QT_VERSION MATCHES 5)
    # 2.8.11+ is required to make Qt5 happy and allow linking QtMain on Windows.
    cmake_minimum_required(VERSION 2.8.11)
else()
    # We probably support older versions than this.
    cmake_minimum_required(VERSION 2.6)
endif()

# Sound setup
find_package(FFmpeg REQUIRED COMPONENTS AVCODEC AVFORMAT AVUTIL SWSCALE SWRESAMPLE)
# Required for building the FFmpeg headers
add_definitions(-D__STDC_CONSTANT_MACROS)

# TinyXML
option(USE_SYSTEM_TINYXML "Use system TinyXML library instead of internal." OFF)
if (USE_SYSTEM_TINYXML)
    find_package(TinyXML REQUIRED)
    add_definitions (-DTIXML_USE_STL)
    include_directories(SYSTEM ${TinyXML_INCLUDE_DIRS})
endif()

# Platform specific
if (WIN32)
    if(NOT MINGW)
    set(Boost_USE_STATIC_LIBS   ON)
    add_definitions(-DBOOST_ALL_NO_LIB)
    endif(NOT MINGW)

    # Suppress WinMain(), provided by SDL
    add_definitions(-DSDL_MAIN_HANDLED)

    # Get rid of useless crud from windows.h
    add_definitions(-DNOMINMAX -DWIN32_LEAN_AND_MEAN)
endif()

if (NOT WIN32 AND BUILD_WIZARD) # windows users can just run the morrowind installer
    find_package(LIBUNSHIELD REQUIRED) # required only for non win32 when building openmw-wizard
    set(OPENMW_USE_UNSHIELD TRUE)
endif()

option(OPENGL_ES "enable opengl es support" FALSE )

if (OPENGL_ES)
    add_definitions(-DOPENGL_ES)
endif(OPENGL_ES)

# Fix for not visible pthreads functions for linker with glibc 2.15
if (UNIX AND NOT APPLE)
    find_package (Threads)
endif()

# Look for stdint.h
include(CheckIncludeFile)
check_include_file(stdint.h HAVE_STDINT_H)
if(NOT HAVE_STDINT_H)
    unset(HAVE_STDINT_H CACHE)
    message(FATAL_ERROR "stdint.h was not found" )
endif()


set(BOOST_COMPONENTS system filesystem program_options thread)
if(WIN32)
    set(BOOST_COMPONENTS ${BOOST_COMPONENTS} locale)
endif(WIN32)

IF(BOOST_STATIC)
    set(Boost_USE_STATIC_LIBS   ON)
endif()

find_package(OpenSceneGraph 3.3.4 REQUIRED osgDB osgViewer osgText osgGA osgAnimation osgParticle osgUtil osgFX)

include_directories(${OPENSCENEGRAPH_INCLUDE_DIRS})

if(OSG_STATIC)
   macro(use_static_osg_plugin_library PLUGIN_NAME)
      set(PLUGIN_NAME_DBG ${PLUGIN_NAME}d ${PLUGIN_NAME}D ${PLUGIN_NAME}_d ${PLUGIN_NAME}_D ${PLUGIN_NAME}_debug ${PLUGIN_NAME})

      # For now, users wishing to do a static build will need to pass the path to where the plugins reside
      # More clever logic would need to deduce the path, probably installed under <OpenSceneGraph>/lib/osgPlugins-<X.X.X>
      include(FindPkgMacros)
      find_library(${PLUGIN_NAME}_LIBRARY_REL NAMES ${PLUGIN_NAME} HINTS ${OSG_PLUGIN_LIB_SEARCH_PATH})
      find_library(${PLUGIN_NAME}_LIBRARY_DBG NAMES ${PLUGIN_NAME_DBG} HINTS ${OSG_PLUGIN_LIB_SEARCH_PATH})
      make_library_set(${PLUGIN_NAME}_LIBRARY)

      if("${${PLUGIN_NAME}_LIBRARY}" STREQUAL "")
         message(FATAL_ERROR "Unable to find static OpenSceneGraph plugin: ${PLUGIN_NAME}")
      endif()

      set(OPENSCENEGRAPH_LIBRARIES ${OPENSCENEGRAPH_LIBRARIES} ${${PLUGIN_NAME}_LIBRARY})
   endmacro()

   add_definitions(-DOSG_LIBRARY_STATIC)

   set(PLUGIN_LIST
       osgdb_png # depends on libpng, zlib
       osgdb_tga
       osgdb_dds
       osgdb_jpeg # depends on libjpeg
       )

   foreach(PLUGIN ${PLUGIN_LIST})
      use_static_osg_plugin_library(${PLUGIN})
   endforeach()

   # OSG static plugins need to linked against their respective dependencies
   set(PLUGIN_DEPS_LIST
       PNG # needed by osgdb_png
       ZLIB # needed by osgdb_png
       JPEG # needed by osgdb_jpeg
       )

   macro(use_static_osg_plugin_dep DEPENDENCY)
      find_package(${DEPENDENCY} REQUIRED)

      set(OPENSCENEGRAPH_LIBRARIES ${OPENSCENEGRAPH_LIBRARIES} ${${DEPENDENCY}_LIBRARIES})
   endmacro()
   foreach(DEPENDENCY ${PLUGIN_DEPS_LIST})
      use_static_osg_plugin_dep(${DEPENDENCY})
   endforeach()
endif()

if(QT_STATIC)
   if(WIN32)
      if(DESIRED_QT_VERSION MATCHES 4)
         # QtCore needs WSAAsyncSelect from Ws2_32.lib
         set(QT_QTCORE_LIBRARY ${QT_QTCORE_LIBRARY} Ws2_32.lib)
         message("QT_QTCORE_LIBRARY: ${QT_QTCORE_LIBRARY}")
      endif()
   endif()
endif()

find_package(MyGUI REQUIRED)
if (${MYGUI_VERSION} VERSION_LESS "3.2.1")
    message(FATAL_ERROR "OpenMW requires MyGUI 3.2.1 or later, please install the latest version from http://mygui.info")
endif()

find_package(Boost REQUIRED COMPONENTS ${BOOST_COMPONENTS})
find_package(SDL2 REQUIRED)
find_package(OpenAL REQUIRED)
find_package(Bullet 283 REQUIRED COMPONENTS BulletCollision LinearMath)

include_directories("."
    SYSTEM
    ${SDL2_INCLUDE_DIR}
    ${Boost_INCLUDE_DIR}
    ${MYGUI_INCLUDE_DIRS}
    ${OPENAL_INCLUDE_DIR}
    ${Bullet_INCLUDE_DIRS}
)

link_directories(${SDL2_LIBRARY_DIRS} ${Boost_LIBRARY_DIRS} ${MYGUI_LIB_DIR})

if(MYGUI_STATIC)
   add_definitions(-DMYGUI_STATIC)
endif (MYGUI_STATIC)

if (APPLE)
    configure_file(${OpenMW_SOURCE_DIR}/files/mac/openmw-Info.plist.in
        "${APP_BUNDLE_DIR}/Contents/Info.plist")

    configure_file(${OpenMW_SOURCE_DIR}/files/mac/openmw.icns
        "${APP_BUNDLE_DIR}/Contents/Resources/OpenMW.icns" COPYONLY)
endif (APPLE)

# Set up DEBUG define
set_directory_properties(PROPERTIES COMPILE_DEFINITIONS_DEBUG DEBUG=1)

add_subdirectory(files/)

# Specify build paths

if (APPLE)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${APP_BUNDLE_DIR}/Contents/MacOS")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${APP_BUNDLE_DIR}/Contents/MacOS")

    if (OPENMW_OSX_DEPLOYMENT)
        SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
    endif()
else (APPLE)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${OpenMW_BINARY_DIR}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${OpenMW_BINARY_DIR}")
endif (APPLE)

# Other files

configure_file(${OpenMW_SOURCE_DIR}/files/settings-default.cfg
    "${OpenMW_BINARY_DIR}/settings-default.cfg")

configure_file(${OpenMW_SOURCE_DIR}/files/openmw.cfg.local
    "${OpenMW_BINARY_DIR}/openmw.cfg")

configure_file(${OpenMW_SOURCE_DIR}/files/openmw.cfg
    "${OpenMW_BINARY_DIR}/openmw.cfg.install")

configure_file(${OpenMW_SOURCE_DIR}/files/openmw-cs.cfg
    "${OpenMW_BINARY_DIR}/openmw-cs.cfg")

configure_file(${OpenMW_SOURCE_DIR}/files/opencs/defaultfilters
    "${OpenMW_BINARY_DIR}/resources/defaultfilters" COPYONLY)

configure_file(${OpenMW_SOURCE_DIR}/files/gamecontrollerdb.txt
    "${OpenMW_BINARY_DIR}/gamecontrollerdb.txt")

if (NOT WIN32 AND NOT APPLE)
    configure_file(${OpenMW_SOURCE_DIR}/files/openmw.desktop
        "${OpenMW_BINARY_DIR}/openmw.desktop")
    configure_file(${OpenMW_SOURCE_DIR}/files/openmw.appdata.xml
        "${OpenMW_BINARY_DIR}/openmw.appdata.xml")
    configure_file(${OpenMW_SOURCE_DIR}/files/openmw-cs.desktop
        "${OpenMW_BINARY_DIR}/openmw-cs.desktop")
endif()

# CXX Compiler settings
if (CMAKE_CXX_COMPILER_ID STREQUAL GNU OR CMAKE_CXX_COMPILER_ID STREQUAL Clang)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wundef -Wno-unused-parameter -std=c++98 -pedantic -Wno-long-long")

    if (CMAKE_CXX_COMPILER_ID STREQUAL Clang AND NOT APPLE)
        if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 3.6 OR CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL 3.6)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-potentially-evaluated-expression")
        endif ()
    endif()

    if (CMAKE_CXX_COMPILER_ID STREQUAL GNU AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.6 OR CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL 4.6)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-but-set-parameter")
    endif()
elseif (MSVC)
    # Enable link-time code generation globally for all linking
    if (OPENMW_LTO_BUILD)
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /GL")
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")
        set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /LTCG")
        set(CMAKE_STATIC_LINKER_FLAGS_RELEASE "${CMAKE_STATIC_LINKER_FLAGS_RELEASE} /LTCG")
    endif()

    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /FORCE:MULTIPLE")
endif (CMAKE_CXX_COMPILER_ID STREQUAL GNU OR CMAKE_CXX_COMPILER_ID STREQUAL Clang)

IF(NOT WIN32 AND NOT APPLE)
    # Linux installation

    # Install binaries
    IF(BUILD_OPENMW)
        INSTALL(PROGRAMS "${OpenMW_BINARY_DIR}/openmw" DESTINATION "${BINDIR}" )
    ENDIF(BUILD_OPENMW)
    IF(BUILD_LAUNCHER)
        INSTALL(PROGRAMS "${OpenMW_BINARY_DIR}/openmw-launcher" DESTINATION "${BINDIR}" )
    ENDIF(BUILD_LAUNCHER)
    IF(BUILD_BSATOOL)
        INSTALL(PROGRAMS "${OpenMW_BINARY_DIR}/bsatool" DESTINATION "${BINDIR}" )
    ENDIF(BUILD_BSATOOL)
    IF(BUILD_ESMTOOL)
        INSTALL(PROGRAMS "${OpenMW_BINARY_DIR}/esmtool" DESTINATION "${BINDIR}" )
    ENDIF(BUILD_ESMTOOL)
    IF(BUILD_NIFTEST)
        INSTALL(PROGRAMS "${OpenMW_BINARY_DIR}/niftest" DESTINATION "${BINDIR}" )
    ENDIF(BUILD_NIFTEST)
    IF(BUILD_MWINIIMPORTER)
        INSTALL(PROGRAMS "${OpenMW_BINARY_DIR}/openmw-iniimporter" DESTINATION "${BINDIR}" )
    ENDIF(BUILD_MWINIIMPORTER)
    IF(BUILD_ESSIMPORTER)
        INSTALL(PROGRAMS "${OpenMW_BINARY_DIR}/openmw-essimporter" DESTINATION "${BINDIR}" )
    ENDIF(BUILD_ESSIMPORTER)
    IF(BUILD_OPENCS)
        INSTALL(PROGRAMS "${OpenMW_BINARY_DIR}/openmw-cs" DESTINATION "${BINDIR}" )
    ENDIF(BUILD_OPENCS)
    IF(BUILD_WIZARD)
        INSTALL(PROGRAMS "${OpenMW_BINARY_DIR}/openmw-wizard" DESTINATION "${BINDIR}" )
    ENDIF(BUILD_WIZARD)
    #if(BUILD_MYGUI_PLUGIN)
    #    INSTALL(PROGRAMS "${OpenMW_BINARY_DIR}/Plugin_MyGUI_OpenMW_Resources.so" DESTINATION "${LIBDIR}" )
    #ENDIF(BUILD_MYGUI_PLUGIN)

    # Install licenses
    INSTALL(FILES "docs/license/DejaVu Font License.txt" DESTINATION "${LICDIR}" )

    # Install icon and desktop file
    INSTALL(FILES "${OpenMW_BINARY_DIR}/openmw.desktop" DESTINATION "${DATAROOTDIR}/applications" COMPONENT "openmw")
    INSTALL(FILES "${OpenMW_SOURCE_DIR}/files/launcher/images/openmw.png" DESTINATION "${ICONDIR}" COMPONENT "openmw")
    INSTALL(FILES "${OpenMW_BINARY_DIR}/openmw.appdata.xml" DESTINATION "${DATAROOTDIR}/appdata" COMPONENT "openmw")
    IF(BUILD_OPENCS)
        INSTALL(FILES "${OpenMW_BINARY_DIR}/openmw-cs.desktop" DESTINATION "${DATAROOTDIR}/applications" COMPONENT "opencs")
        INSTALL(FILES "${OpenMW_SOURCE_DIR}/files/opencs/openmw-cs.png" DESTINATION "${ICONDIR}" COMPONENT "opencs")
    ENDIF(BUILD_OPENCS)

    # Install global configuration files
    INSTALL(FILES "${OpenMW_BINARY_DIR}/settings-default.cfg" DESTINATION "${SYSCONFDIR}" COMPONENT "openmw")
    INSTALL(FILES "${OpenMW_BINARY_DIR}/openmw.cfg.install" DESTINATION "${SYSCONFDIR}" RENAME "openmw.cfg" COMPONENT "openmw")
    INSTALL(FILES "${OpenMW_BINARY_DIR}/resources/version" DESTINATION "${SYSCONFDIR}" COMPONENT "openmw")
    INSTALL(FILES "${OpenMW_BINARY_DIR}/gamecontrollerdb.txt" DESTINATION "${SYSCONFDIR}" COMPONENT "openmw")

    IF(BUILD_OPENCS)
        INSTALL(FILES "${OpenMW_BINARY_DIR}/openmw-cs.cfg" DESTINATION "${SYSCONFDIR}" COMPONENT "opencs")
    ENDIF(BUILD_OPENCS)

    # Install resources
    INSTALL(DIRECTORY "${OpenMW_BINARY_DIR}/resources" DESTINATION "${DATADIR}" COMPONENT "Resources")
    INSTALL(DIRECTORY DESTINATION "${DATADIR}/data" COMPONENT "Resources")
ENDIF(NOT WIN32 AND NOT APPLE)

if(WIN32)
    FILE(GLOB dll_files "${OpenMW_BINARY_DIR}/Release/*.dll")
    INSTALL(FILES ${dll_files} DESTINATION ".")
    INSTALL(FILES "${OpenMW_BINARY_DIR}/openmw.cfg.install" DESTINATION "." RENAME "openmw.cfg")
    INSTALL(FILES "${OpenMW_SOURCE_DIR}/CHANGELOG.md" DESTINATION "." RENAME "CHANGELOG.txt")
    INSTALL(FILES "${OpenMW_SOURCE_DIR}/README.md" DESTINATION "." RENAME "README.txt")
    INSTALL(FILES
        "${OpenMW_SOURCE_DIR}/Docs/license/GPL3.txt"
        "${OpenMW_SOURCE_DIR}/Docs/license/DejaVu Font License.txt"
        "${OpenMW_BINARY_DIR}/settings-default.cfg"
        "${OpenMW_BINARY_DIR}/gamecontrollerdb.txt"
        "${OpenMW_BINARY_DIR}/Release/openmw.exe"
        DESTINATION ".")

    IF(BUILD_LAUNCHER)
        INSTALL(PROGRAMS "${OpenMW_BINARY_DIR}/Release/openmw-launcher.exe" DESTINATION ".")
    ENDIF(BUILD_LAUNCHER)
    IF(BUILD_MWINIIMPORTER)
        INSTALL(PROGRAMS "${OpenMW_BINARY_DIR}/Release/openmw-iniimporter.exe" DESTINATION ".")
    ENDIF(BUILD_MWINIIMPORTER)
    IF(BUILD_ESSIMPORTER)
        INSTALL(PROGRAMS "${OpenMW_BINARY_DIR}/Release/openmw-essimporter.exe" DESTINATION ".")
    ENDIF(BUILD_ESSIMPORTER)
    IF(BUILD_OPENCS)
        INSTALL(PROGRAMS "${OpenMW_BINARY_DIR}/Release/openmw-cs.exe" DESTINATION ".")
        INSTALL(FILES "${OpenMW_BINARY_DIR}/openmw-cs.cfg" DESTINATION ".")
    ENDIF(BUILD_OPENCS)
    IF(BUILD_WIZARD)
        INSTALL(PROGRAMS "${OpenMW_BINARY_DIR}/Release/openmw-wizard.exe" DESTINATION ".")
    ENDIF(BUILD_WIZARD)
    if(BUILD_MYGUI_PLUGIN)
        INSTALL(PROGRAMS "${OpenMW_BINARY_DIR}/Release/Plugin_MyGUI_OpenMW_Resources.dll" DESTINATION ".")
    ENDIF(BUILD_MYGUI_PLUGIN)

    INSTALL(DIRECTORY "${OpenMW_BINARY_DIR}/resources" DESTINATION ".")
    FILE(GLOB plugin_dir "${OpenMW_BINARY_DIR}/Release/osgPlugins-*")
    INSTALL(DIRECTORY ${plugin_dir} DESTINATION ".")

    SET(CPACK_GENERATOR "NSIS")
    SET(CPACK_PACKAGE_NAME "OpenMW")
    SET(CPACK_PACKAGE_VENDOR "OpenMW.org")
    SET(CPACK_PACKAGE_VERSION ${OPENMW_VERSION})
    SET(CPACK_PACKAGE_VERSION_MAJOR ${OPENMW_VERSION_MAJOR})
    SET(CPACK_PACKAGE_VERSION_MINOR ${OPENMW_VERSION_MINOR})
    SET(CPACK_PACKAGE_VERSION_PATCH ${OPENMW_VERSION_RELEASE})
    SET(CPACK_PACKAGE_EXECUTABLES "openmw;OpenMW")
    IF(BUILD_LAUNCHER)
        SET(CPACK_PACKAGE_EXECUTABLES "${CPACK_PACKAGE_EXECUTABLES};openmw-launcher;OpenMW Launcher")
    ENDIF(BUILD_LAUNCHER)
    IF(BUILD_OPENCS)
        SET(CPACK_PACKAGE_EXECUTABLES "${CPACK_PACKAGE_EXECUTABLES};openmw-cs;OpenMW Construction Set")
    ENDIF(BUILD_OPENCS)
    IF(BUILD_WIZARD)
        SET(CPACK_PACKAGE_EXECUTABLES "${CPACK_PACKAGE_EXECUTABLES};openmw-wizard;OpenMW Wizard")
    ENDIF(BUILD_WIZARD)
    SET(CPACK_NSIS_CREATE_ICONS_EXTRA "CreateShortCut '\$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\Readme.lnk' '\$INSTDIR\\\\README.txt'")
    SET(CPACK_NSIS_DELETE_ICONS_EXTRA "
        !insertmacro MUI_STARTMENU_GETFOLDER Application $MUI_TEMP
        Delete \\\"$SMPROGRAMS\\\\$MUI_TEMP\\\\Readme.lnk\\\"
        ")
    SET(CPACK_RESOURCE_FILE_README "${OpenMW_SOURCE_DIR}/README.md")
    SET(CPACK_PACKAGE_DESCRIPTION_FILE "${OpenMW_SOURCE_DIR}/README.md")
    SET(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")
    SET(CPACK_NSIS_DISPLAY_NAME "OpenMW ${OPENMW_VERSION}")
    SET(CPACK_NSIS_HELP_LINK "http:\\\\\\\\www.openmw.org")
    SET(CPACK_NSIS_URL_INFO_ABOUT "http:\\\\\\\\www.openmw.org")
    SET(CPACK_NSIS_INSTALLED_ICON_NAME "openmw-launcher.exe")
    SET(CPACK_NSIS_MUI_ICON "${OpenMW_SOURCE_DIR}/files/windows/openmw.ico")
    SET(CPACK_NSIS_MUI_UNIICON "${OpenMW_SOURCE_DIR}/files/windows/openmw.ico")
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
add_subdirectory (extern/osg-ffmpeg-videoplayer)
add_subdirectory (extern/oics)
if (USE_QT)
    add_subdirectory (extern/osgQt)
endif()

# Components
add_subdirectory (components)

# Plugins
#if (BUILD_MYGUI_PLUGIN)
#    add_subdirectory(plugins/mygui_resource_plugin)
#endif()

# Apps and tools
if (BUILD_OPENMW)
    add_subdirectory( apps/openmw )
endif()

if (BUILD_BSATOOL)
  add_subdirectory( apps/bsatool )
endif()

if (BUILD_ESMTOOL)
  add_subdirectory( apps/esmtool )
endif()

if (BUILD_LAUNCHER)
   add_subdirectory( apps/launcher )
endif()

if (BUILD_MWINIIMPORTER)
   add_subdirectory( apps/mwiniimporter )
endif()

if (BUILD_ESSIMPORTER)
   add_subdirectory (apps/essimporter )
endif()

if (BUILD_OPENCS)
   add_subdirectory (apps/opencs)
endif()

if (BUILD_WIZARD)
   add_subdirectory(apps/wizard)
endif()

if (BUILD_NIFTEST)
    add_subdirectory(apps/niftest)
endif(BUILD_NIFTEST)

# UnitTests
if (BUILD_UNITTESTS)
  add_subdirectory( apps/openmw_test_suite )
endif()

if (WIN32)
  if (MSVC)
    if (OPENMW_MP_BUILD)
        set( MT_BUILD "/MP")
    endif()

    foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
        string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
        set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "$(SolutionDir)$(Configuration)" )
        set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "$(ProjectDir)$(Configuration)" )
    endforeach( OUTPUTCONFIG )

    if (USE_DEBUG_CONSOLE AND BUILD_OPENMW)
      set_target_properties(openmw PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE")
      set_target_properties(openmw PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:CONSOLE")
      set_target_properties(openmw PROPERTIES COMPILE_DEFINITIONS_DEBUG "_CONSOLE")
    elseif (BUILD_OPENMW)
      # Turn off debug console, debug output will be written to visual studio output instead
      set_target_properties(openmw PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:WINDOWS")
      set_target_properties(openmw PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:WINDOWS")
    endif()

    if (BUILD_OPENMW)
        # Release builds use the debug console
        set_target_properties(openmw PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:CONSOLE")
        set_target_properties(openmw PROPERTIES COMPILE_DEFINITIONS_RELEASE "_CONSOLE")
        set_target_properties(openmw PROPERTIES LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:CONSOLE")
    endif()

    # Play a bit with the warning levels

    set(WARNINGS "/Wall") # Since windows can only disable specific warnings, not enable them

    set(WARNINGS_DISABLE
        # Warnings that aren't enabled normally and don't need to be enabled
        # They're unneeded and sometimes completely retarded warnings that /Wall enables
        # Not going to bother commenting them as they tend to warn on every standard library file
        4061 4263 4264 4266 4350 4371 4435 4514 4548 4571 4610 4619 4623 4625 4626 4628 4640 4668 4710 4711 4820 4826 4917 4946

        # Warnings that are thrown on standard libraries and not OpenMW
        4347 # Non-template function with same name and parameter count as template function
        4365 # Variable signed/unsigned mismatch
        4510 4512 # Unable to generate copy constructor/assignment operator as it's not public in the base
        4706 # Assignment in conditional expression
        4738 # Storing 32-bit float result in memory, possible loss of performance
        4986 # Undocumented warning that occurs in the crtdbg.h file
        4987 # nonstandard extension used (triggered by setjmp.h)
        4996 # Function was declared deprecated

        # caused by boost
        4191 # 'type cast' : unsafe conversion (1.56, thread_primitives.hpp, normally off)

        # OpenMW specific warnings
        4099 # Type mismatch, declared class or struct is defined with other type
        4100 # Unreferenced formal parameter (-Wunused-parameter)
        4101 # Unreferenced local variable (-Wunused-variable)
        4127 # Conditional expression is constant
        4242 # Storing value in a variable of a smaller type, possible loss of data
        4244 # Storing value of one type in variable of another (size_t in int, for example)
        4245 # Signed/unsigned mismatch
        4267 # Conversion from 'size_t' to 'int', possible loss of data
        4305 # Truncating value (double to float, for example)
        4309 # Variable overflow, trying to store 128 in a signed char for example
        4351 # New behavior: elements of array 'array' will be default initialized (desired behavior)
        4355 # Using 'this' in member initialization list
        4505 # Unreferenced local function has been removed
        4701 # Potentially uninitialized local variable used
        4702 # Unreachable code
        4800 # Boolean optimization warning, e.g. myBool = (myInt != 0) instead of myBool = myInt
        )

    foreach(d ${WARNINGS_DISABLE})
        set(WARNINGS "${WARNINGS} /wd${d}")
    endforeach(d)

    set_target_properties(components PROPERTIES COMPILE_FLAGS "${WARNINGS} ${MT_BUILD}")
    set_target_properties(osg-ffmpeg-videoplayer PROPERTIES COMPILE_FLAGS "${WARNINGS} ${MT_BUILD}")

    if (BUILD_BSATOOL)
        set_target_properties(bsatool PROPERTIES COMPILE_FLAGS "${WARNINGS} ${MT_BUILD}")
    endif()

    if (BUILD_ESMTOOL)
        set_target_properties(esmtool PROPERTIES COMPILE_FLAGS "${WARNINGS} ${MT_BUILD}")
    endif()

    if (BUILD_ESSIMPORTER)
        set_target_properties(openmw-essimporter PROPERTIES COMPILE_FLAGS "${WARNINGS} ${MT_BUILD}")
    endif()

    if (BUILD_LAUNCHER)
        set_target_properties(openmw-launcher PROPERTIES COMPILE_FLAGS "${WARNINGS} ${MT_BUILD}")
    endif()

    if (BUILD_MWINIIMPORTER)
        set_target_properties(openmw-iniimporter PROPERTIES COMPILE_FLAGS "${WARNINGS} ${MT_BUILD}")
    endif()

    if (BUILD_OPENCS)
        set_target_properties(openmw-cs PROPERTIES COMPILE_FLAGS "${WARNINGS} ${MT_BUILD}")
    endif()

    if (BUILD_OPENMW)
        set_target_properties(openmw PROPERTIES COMPILE_FLAGS "${WARNINGS} ${MT_BUILD}")
    endif()

    if (BUILD_WIZARD)
        set_target_properties(openmw-wizard PROPERTIES COMPILE_FLAGS "${WARNINGS} ${MT_BUILD}")
    endif()
  endif(MSVC)

  # TODO: At some point release builds should not use the console but rather write to a log file
  #set_target_properties(openmw PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")
  #set_target_properties(openmw PROPERTIES LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:WINDOWS")
endif()

# Apple bundling
if (APPLE)
    get_property(QT_COCOA_PLUGIN_PATH TARGET Qt5::QCocoaIntegrationPlugin PROPERTY LOCATION_RELEASE)
    get_filename_component(QT_COCOA_PLUGIN_DIR "${QT_COCOA_PLUGIN_PATH}" DIRECTORY)
    get_filename_component(QT_COCOA_PLUGIN_GROUP "${QT_COCOA_PLUGIN_DIR}" NAME)
    get_filename_component(QT_COCOA_PLUGIN_NAME "${QT_COCOA_PLUGIN_PATH}" NAME)
    configure_file("${QT_COCOA_PLUGIN_PATH}" "${APP_BUNDLE_DIR}/Contents/MacOS/${QT_COCOA_PLUGIN_GROUP}/${QT_COCOA_PLUGIN_NAME}" COPYONLY)

    if (BUILD_OPENCS)
      get_property(OPENCS_BUNDLE_NAME_TMP TARGET openmw-cs PROPERTY OUTPUT_NAME)
      set(OPENCS_BUNDLE_NAME "${OPENCS_BUNDLE_NAME_TMP}.app")
      configure_file("${QT_COCOA_PLUGIN_PATH}" "${OPENCS_BUNDLE_NAME}/Contents/MacOS/${QT_COCOA_PLUGIN_GROUP}/${QT_COCOA_PLUGIN_NAME}" COPYONLY)
    endif ()

    set(INSTALL_SUBDIR OpenMW)

    install(DIRECTORY "${APP_BUNDLE_DIR}" USE_SOURCE_PERMISSIONS DESTINATION "${INSTALL_SUBDIR}" COMPONENT Runtime)
    install(DIRECTORY "${OpenMW_BINARY_DIR}/resources" DESTINATION "${INSTALL_SUBDIR}" COMPONENT Runtime)
    install(FILES "${OpenMW_BINARY_DIR}/openmw.cfg.install" RENAME "openmw.cfg" DESTINATION "${INSTALL_SUBDIR}" COMPONENT Runtime)
    install(FILES "${OpenMW_BINARY_DIR}/settings-default.cfg" DESTINATION "${INSTALL_SUBDIR}" COMPONENT Runtime)
    install(FILES "${OpenMW_BINARY_DIR}/gamecontrollerdb.txt" DESTINATION "${INSTALL_SUBDIR}" COMPONENT Runtime)
    install(FILES "${OpenMW_BINARY_DIR}/openmw-cs.cfg" DESTINATION "${INSTALL_SUBDIR}" COMPONENT Runtime)

    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_PACKAGE_VERSION ${OPENMW_VERSION})
    set(CPACK_PACKAGE_VERSION_MAJOR ${OPENMW_VERSION_MAJOR})
    set(CPACK_PACKAGE_VERSION_MINOR ${OPENMW_VERSION_MINOR})
    set(CPACK_PACKAGE_VERSION_PATCH ${OPENMW_VERSION_RELEASE})

    set(INSTALLED_OPENMW_APP "\${CMAKE_INSTALL_PREFIX}/${INSTALL_SUBDIR}/${APP_BUNDLE_NAME}")
    set(INSTALLED_OPENCS_APP "\${CMAKE_INSTALL_PREFIX}/${INSTALL_SUBDIR}/${OPENCS_BUNDLE_NAME}")

    install(CODE "
        set(BU_CHMOD_BUNDLE_ITEMS ON)
        set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH})
        include(BundleUtilities)
        cmake_minimum_required(VERSION 3.1)
    " COMPONENT Runtime)

    set(ABSOLUTE_PLUGINS "")
    set(USED_OSG_PLUGINS
                        osgdb_dds
                        osgdb_jpeg
                        osgdb_png
                        osgdb_tga
                      )

    foreach (PLUGIN_NAME ${USED_OSG_PLUGINS})
      set(PLUGIN_ABS "${OSG_PLUGIN_LIB_SEARCH_PATH}/${PLUGIN_NAME}.so")
      set(ABSOLUTE_PLUGINS ${PLUGIN_ABS} ${ABSOLUTE_PLUGINS})
    endforeach ()

    get_filename_component(OSG_PLUGIN_PREFIX_DIR "${OSG_PLUGIN_LIB_SEARCH_PATH}" NAME)

    # installs used plugins in bundle at given path (bundle_path must be relative to ${CMAKE_INSTALL_PREFIX})
    # and returns list of install paths for all installed plugins
    function (install_plugins_for_bundle bundle_path plugins_var)
        set(RELATIVE_PLUGIN_INSTALL_BASE "${bundle_path}/Contents/PlugIns/${OSG_PLUGIN_PREFIX_DIR}")

        set(PLUGINS "")
        set(PLUGIN_INSTALL_BASE "\${CMAKE_INSTALL_PREFIX}/${RELATIVE_PLUGIN_INSTALL_BASE}")

        foreach (PLUGIN ${ABSOLUTE_PLUGINS})
            get_filename_component(PLUGIN_RELATIVE ${PLUGIN} NAME)
            get_filename_component(PLUGIN_RELATIVE_WE ${PLUGIN} NAME_WE)

            set(PLUGIN_DYLIB_IN_BUNDLE "${PLUGIN_INSTALL_BASE}/${PLUGIN_RELATIVE}")
            set(PLUGINS ${PLUGINS} "${PLUGIN_DYLIB_IN_BUNDLE}")

            install(CODE "
                copy_resolved_item_into_bundle(\"${PLUGIN}\" \"${PLUGIN_DYLIB_IN_BUNDLE}\")
            " COMPONENT Runtime)
        endforeach ()

        set(${plugins_var} ${PLUGINS} PARENT_SCOPE)
    endfunction (install_plugins_for_bundle)

    install_plugins_for_bundle("${INSTALL_SUBDIR}/${APP_BUNDLE_NAME}" PLUGINS)
    install_plugins_for_bundle("${INSTALL_SUBDIR}/${OPENCS_BUNDLE_NAME}" OPENCS_PLUGINS)

    set(PLUGINS ${PLUGINS} "${INSTALLED_OPENMW_APP}/Contents/MacOS/${QT_COCOA_PLUGIN_GROUP}/${QT_COCOA_PLUGIN_NAME}")
    set(OPENCS_PLUGINS ${OPENCS_PLUGINS} "${INSTALLED_OPENCS_APP}/Contents/MacOS/${QT_COCOA_PLUGIN_GROUP}/${QT_COCOA_PLUGIN_NAME}")

    install(CODE "
        function(gp_item_default_embedded_path_override item  default_embedded_path_var)
            if (\${item} MATCHES ${OSG_PLUGIN_PREFIX_DIR})
              set(path \"@executable_path/../PlugIns/${OSG_PLUGIN_PREFIX_DIR}\")
              set(\${default_embedded_path_var} \"\${path}\" PARENT_SCOPE)
            endif()
        endfunction()

        cmake_policy(SET CMP0009 OLD)
        fixup_bundle(\"${INSTALLED_OPENMW_APP}\" \"${PLUGINS}\" \"\")
        fixup_bundle(\"${INSTALLED_OPENCS_APP}\" \"${OPENCS_PLUGINS}\" \"\")
        " COMPONENT Runtime)
    include(CPack)
endif (APPLE)

# Doxygen Target -- simply run 'make doc' or 'make doc_pages'
# output directory for 'make doc'       is "${OpenMW_BINARY_DIR}/docs/Doxygen"
# output directory for 'make doc_pages' is "${DOXYGEN_PAGES_OUTPUT_DIR}" if defined
#                                       or "${OpenMW_BINARY_DIR}/docs/Pages" otherwise
find_package(Doxygen)
if (DOXYGEN_FOUND)
    # determine output directory for doc_pages
    if (NOT DEFINED DOXYGEN_PAGES_OUTPUT_DIR)
        set(DOXYGEN_PAGES_OUTPUT_DIR "${OpenMW_BINARY_DIR}/docs/Pages")
    endif ()
    configure_file(${OpenMW_SOURCE_DIR}/docs/Doxyfile.cmake ${OpenMW_BINARY_DIR}/docs/Doxyfile @ONLY)
    configure_file(${OpenMW_SOURCE_DIR}/docs/DoxyfilePages.cmake ${OpenMW_BINARY_DIR}/docs/DoxyfilePages @ONLY)
    add_custom_target(doc
        ${DOXYGEN_EXECUTABLE} ${OpenMW_BINARY_DIR}/docs/Doxyfile
        WORKING_DIRECTORY ${OpenMW_BINARY_DIR}
        COMMENT "Generating Doxygen documentation at ${OpenMW_BINARY_DIR}/docs/Doxygen"
        VERBATIM)
    add_custom_target(doc_pages
        ${DOXYGEN_EXECUTABLE} ${OpenMW_BINARY_DIR}/docs/DoxyfilePages
        WORKING_DIRECTORY ${OpenMW_BINARY_DIR}
        COMMENT "Generating documentation for the github-pages at ${DOXYGEN_PAGES_OUTPUT_DIR}" VERBATIM)
endif ()
