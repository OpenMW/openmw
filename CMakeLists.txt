project(OpenMW)
cmake_minimum_required(VERSION 3.1.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# for link time optimization, remove if cmake version is >= 3.9
if(POLICY CMP0069) # LTO
  cmake_policy(SET CMP0069 NEW)
endif()

# for position-independent executable, remove if cmake version is >= 3.14
if(POLICY CMP0083)
  cmake_policy(SET CMP0083 NEW)
endif()

# to link with freetype library
if(POLICY CMP0079)
  cmake_policy(SET CMP0079 NEW)
endif()

option(OPENMW_GL4ES_MANUAL_INIT "Manually initialize gl4es. This is more reliable on platforms without a windowing system. Requires gl4es to be configured with -DNOEGL=ON -DNO_LOADER=ON -DNO_INIT_CONSTRUCTOR=ON." OFF)
if(OPENMW_GL4ES_MANUAL_INIT)
    add_definitions(-DOPENMW_GL4ES_MANUAL_INIT)
endif()

# Apps and tools
option(BUILD_OPENMW             "Build OpenMW" ON)
option(BUILD_LAUNCHER           "Build Launcher" ON)
option(BUILD_WIZARD             "Build Installation Wizard" ON)
option(BUILD_MWINIIMPORTER      "Build MWiniImporter" ON)
option(BUILD_OPENCS             "Build OpenMW Construction Set" ON)
option(BUILD_ESSIMPORTER        "Build ESS (Morrowind save game) importer" ON)
option(BUILD_BSATOOL            "Build BSA extractor" ON)
option(BUILD_ESMTOOL            "Build ESM inspector" ON)
option(BUILD_NIFTEST            "Build nif file tester" ON)
option(BUILD_DOCS               "Build documentation." OFF )
option(BUILD_WITH_CODE_COVERAGE "Enable code coverage with gconv" OFF)
option(BUILD_UNITTESTS          "Enable Unittests with Google C++ Unittest" OFF)
option(BUILD_BENCHMARKS         "Build benchmarks with Google Benchmark" OFF)

set(OpenGL_GL_PREFERENCE LEGACY)  # Use LEGACY as we use GL2; GLNVD is for GL3 and up.

if (NOT BUILD_LAUNCHER AND NOT BUILD_OPENCS AND NOT BUILD_WIZARD)
   set(USE_QT FALSE)
else()
   set(USE_QT TRUE)
endif()

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
endif()

# Version
message(STATUS "Configuring OpenMW...")

set(OPENMW_VERSION_MAJOR 0)
set(OPENMW_VERSION_MINOR 48)
set(OPENMW_VERSION_RELEASE 0)

set(OPENMW_VERSION_COMMITHASH "")
set(OPENMW_VERSION_TAGHASH "")
set(OPENMW_VERSION_COMMITDATE "")

set(OPENMW_VERSION "${OPENMW_VERSION_MAJOR}.${OPENMW_VERSION_MINOR}.${OPENMW_VERSION_RELEASE}")

set(OPENMW_DOC_BASEURL "https://openmw.readthedocs.io/en/stable/")

set(GIT_CHECKOUT FALSE)
if(EXISTS ${PROJECT_SOURCE_DIR}/.git)
    find_package(Git)

    if(GIT_FOUND)
        set(GIT_CHECKOUT TRUE)
    else(GIT_FOUND)
        message(WARNING "Git executable not found")
    endif(GIT_FOUND)

    if(GIT_FOUND)
        execute_process (
                COMMAND ${GIT_EXECUTABLE} log -1 --format='%aI'
                WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                RESULT_VARIABLE EXITCODE3
                OUTPUT_VARIABLE OPENMW_VERSION_COMMITDATE
                OUTPUT_STRIP_TRAILING_WHITESPACE)
        if(NOT EXITCODE3)
            string(SUBSTRING ${OPENMW_VERSION_COMMITDATE} 1 10 OPENMW_VERSION_COMMITDATE)
        endif(NOT EXITCODE3)
    endif(GIT_FOUND)
endif(EXISTS ${PROJECT_SOURCE_DIR}/.git)

# Macros
include(OpenMWMacros)
include(WholeArchive)

# doxygen main page

configure_file ("${OpenMW_SOURCE_DIR}/docs/mainpage.hpp.cmake" "${OpenMW_BINARY_DIR}/docs/mainpage.hpp")

option(BOOST_STATIC "Link static build of Boost into the binaries" FALSE)
option(SDL2_STATIC "Link static build of SDL into the binaries" FALSE)
option(QT_STATIC "Link static build of QT into the binaries" FALSE)

option(OPENMW_USE_SYSTEM_BULLET "Use system provided bullet physics library" ON)
if(OPENMW_USE_SYSTEM_BULLET)
    set(_bullet_static_default OFF)
else()
    set(_bullet_static_default ON)
endif()
option(BULLET_STATIC "Link static build of Bullet into the binaries" ${_bullet_static_default})

option(OPENMW_USE_SYSTEM_OSG "Use system provided OpenSceneGraph libraries" ON)
if(OPENMW_USE_SYSTEM_OSG)
    set(_osg_static_default OFF)
else()
    set(_osg_static_default ON)
endif()
option(OSG_STATIC "Link static build of OpenSceneGraph into the binaries" ${_osg_static_default})

option(OPENMW_USE_SYSTEM_MYGUI "Use system provided mygui library" ON)
if(OPENMW_USE_SYSTEM_MYGUI)
    set(_mygui_static_default OFF)
else()
    set(_mygui_static_default ON)
endif()
option(MYGUI_STATIC "Link static build of Mygui into the binaries" ${_mygui_static_default})

option(OPENMW_USE_SYSTEM_RECASTNAVIGATION "Use system provided recastnavigation library" OFF)
if(OPENMW_USE_SYSTEM_RECASTNAVIGATION)
    set(_recastnavigation_static_default OFF)
    find_package(RecastNavigation REQUIRED)
else()
    set(_recastnavigation_static_default ON)
endif()
option(RECASTNAVIGATION_STATIC "Build recastnavigation static libraries" ${_recastnavigation_static_default})

option(OPENMW_USE_SYSTEM_SQLITE3 "Use system provided SQLite3 library" ON)

option(OPENMW_UNITY_BUILD "Use fewer compilation units to speed up compile time" FALSE)
option(OPENMW_LTO_BUILD "Build OpenMW with Link-Time Optimization (Needs ~2GB of RAM)" OFF)

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
endif()

# Set up common paths
if (APPLE)
    set(MORROWIND_DATA_FILES "./data" CACHE PATH "location of Morrowind data files")
    set(OPENMW_RESOURCE_FILES "../Resources/resources" CACHE PATH "location of OpenMW resources files")
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

if(MSVC)
  add_compile_options("/utf-8")
endif()

# Dependencies
find_package(OpenGL REQUIRED)

find_package(LZ4 REQUIRED)

if (USE_QT)
    find_package(Qt5Core 5.12 REQUIRED)
    find_package(Qt5Widgets REQUIRED)
    find_package(Qt5Network REQUIRED)
    find_package(Qt5OpenGL REQUIRED)
endif()

set(USED_OSG_COMPONENTS
    osgDB
    osgViewer
    osgText
    osgGA
    osgParticle
    osgUtil
    osgFX
    osgShadow
    osgAnimation)
set(USED_OSG_PLUGINS
    osgdb_bmp
    osgdb_dds
    osgdb_freetype
    osgdb_jpeg
    osgdb_osg
    osgdb_png
    osgdb_serializers_osg
    osgdb_tga)

add_subdirectory(extern)

# Sound setup

# Require at least ffmpeg 3.2 for now
SET(FFVER_OK FALSE)

find_package(FFmpeg REQUIRED COMPONENTS AVCODEC AVFORMAT AVUTIL SWSCALE SWRESAMPLE)

if(FFmpeg_FOUND)
    SET(FFVER_OK TRUE)

    # Can not detect FFmpeg version on Windows for now
    if (NOT WIN32)
        if(FFmpeg_AVFORMAT_VERSION VERSION_LESS "57.56.100")
            message(STATUS "libavformat is too old! (${FFmpeg_AVFORMAT_VERSION}, wanted 57.56.100)")
            set(FFVER_OK FALSE)
        endif()
        if(FFmpeg_AVCODEC_VERSION VERSION_LESS "57.64.100")
            message(STATUS "libavcodec is too old! (${FFmpeg_AVCODEC_VERSION}, wanted 57.64.100)")
            set(FFVER_OK FALSE)
        endif()
        if(FFmpeg_AVUTIL_VERSION VERSION_LESS "55.34.100")
            message(STATUS "libavutil is too old! (${FFmpeg_AVUTIL_VERSION}, wanted 55.34.100)")
            set(FFVER_OK FALSE)
        endif()
        if(FFmpeg_SWSCALE_VERSION VERSION_LESS "4.2.100")
            message(STATUS "libswscale is too old! (${FFmpeg_SWSCALE_VERSION}, wanted 4.2.100)")
            set(FFVER_OK FALSE)
        endif()
        if(FFmpeg_SWRESAMPLE_VERSION VERSION_LESS "2.3.100")
            message(STATUS "libswresample is too old! (${FFmpeg_SWRESAMPLE_VERSION}, wanted 2.3.100)")
            set(FFVER_OK FALSE)
        endif()
    endif()

    if(NOT FFVER_OK AND NOT APPLE)  # unable to detect on version on MacOS < 11.0
        message(FATAL_ERROR "FFmpeg version is too old, 3.2 is required" )
    endif()
endif()

if(NOT FFmpeg_FOUND)
    message(FATAL_ERROR "FFmpeg was not found" )
endif()

if(WIN32)
    message("Can not detect FFmpeg version, at least the 3.2 is required" )
endif()

# Required for building the FFmpeg headers
add_definitions(-D__STDC_CONSTANT_MACROS)

# Reqiuired for unity build
add_definitions(-DMYGUI_DONT_REPLACE_NULLPTR)

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

if(OPENMW_USE_SYSTEM_BULLET)
    set(REQUIRED_BULLET_VERSION 286) # Bullet 286 required due to runtime bugfixes for btCapsuleShape
    if (DEFINED ENV{TRAVIS_BRANCH} OR DEFINED ENV{APPVEYOR})
        set(REQUIRED_BULLET_VERSION 283) # but for build testing, 283 is fine
    endif()

    # First, try BulletConfig-float64.cmake which comes with Debian derivatives.
    # This file does not define the Bullet version in a CMake-friendly way.
    find_package(Bullet CONFIGS BulletConfig-float64.cmake QUIET COMPONENTS BulletCollision LinearMath)
    if (BULLET_FOUND)
        string(REPLACE "." "" _bullet_version_num ${BULLET_VERSION_STRING})
        if (_bullet_version_num VERSION_LESS REQUIRED_BULLET_VERSION)
            message(FATAL_ERROR "System bullet version too old, OpenMW requires at least ${REQUIRED_BULLET_VERSION}, got ${_bullet_version_num}")
        endif()
        # Fix the relative include:
        set(BULLET_INCLUDE_DIRS "${BULLET_ROOT_DIR}/${BULLET_INCLUDE_DIRS}")
        include(FindPackageMessage)
        find_package_message(Bullet "Found Bullet: ${BULLET_LIBRARIES} ${BULLET_VERSION_STRING}" "${BULLET_VERSION_STRING}-float64")
    else()
        find_package(Bullet ${REQUIRED_BULLET_VERSION} REQUIRED COMPONENTS BulletCollision LinearMath)
    endif()

    # Only link the Bullet libraries that we need:
    string(REGEX MATCHALL "((optimized|debug);)?[^;]*(BulletCollision|LinearMath)[^;]*" BULLET_LIBRARIES "${BULLET_LIBRARIES}")

    include(cmake/CheckBulletPrecision.cmake)
    if (HAS_DOUBLE_PRECISION_BULLET)
        message(STATUS "Bullet uses double precision")
    else()
        message(FATAL_ERROR "Bullet does not uses double precision")
    endif()
endif()

if (NOT WIN32 AND BUILD_WIZARD) # windows users can just run the morrowind installer
    find_package(LIBUNSHIELD REQUIRED) # required only for non win32 when building openmw-wizard
    set(OPENMW_USE_UNSHIELD TRUE)
endif()

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

if(OPENMW_USE_SYSTEM_OSG)
    find_package(OpenSceneGraph 3.4.0 REQUIRED ${USED_OSG_COMPONENTS})
    if (${OPENSCENEGRAPH_VERSION} VERSION_GREATER 3.6.2 AND ${OPENSCENEGRAPH_VERSION} VERSION_LESS 3.6.5)
        message(FATAL_ERROR "OpenSceneGraph version ${OPENSCENEGRAPH_VERSION} has critical regressions which cause crashes. Please upgrade to 3.6.5 or later. We strongly recommend using the tip of the official 'OpenSceneGraph-3.6' branch or the tip of '3.6' OpenMW/osg (OSGoS).")
    endif()

    if(OSG_STATIC)
        find_package(OSGPlugins REQUIRED COMPONENTS ${USED_OSG_PLUGINS})
    endif()
endif()

include_directories(BEFORE SYSTEM ${OPENSCENEGRAPH_INCLUDE_DIRS})

if(OSG_STATIC)
    add_definitions(-DOSG_LIBRARY_STATIC)
endif()

set(BOOST_COMPONENTS system filesystem program_options iostreams)
if(WIN32)
    set(BOOST_COMPONENTS ${BOOST_COMPONENTS} locale)
    if(MSVC)
        # boost-zlib is not present (nor needed) in vcpkg version of boost.
        # there, it is part of boost-iostreams instead.
        set(BOOST_OPTIONAL_COMPONENTS zlib)
    endif(MSVC)
endif(WIN32)

IF(BOOST_STATIC)
    set(Boost_USE_STATIC_LIBS   ON)
endif()

set(Boost_NO_BOOST_CMAKE ON)

find_package(Boost 1.6.2 REQUIRED COMPONENTS ${BOOST_COMPONENTS} OPTIONAL_COMPONENTS ${BOOST_OPTIONAL_COMPONENTS})
if(OPENMW_USE_SYSTEM_MYGUI)
    find_package(MyGUI 3.2.2 REQUIRED)
endif()
find_package(SDL2 2.0.9 REQUIRED)
find_package(OpenAL REQUIRED)

option(USE_LUAJIT "Switch Lua/LuaJit (TRUE is highly recommended)" TRUE)
if(USE_LUAJIT)
   find_package(LuaJit REQUIRED)
   set(LUA_INCLUDE_DIR ${LuaJit_INCLUDE_DIR})
   set(LUA_LIBRARIES ${LuaJit_LIBRARIES})
else(USE_LUAJIT)
   find_package(Lua REQUIRED)
   add_compile_definitions(NO_LUAJIT)
endif(USE_LUAJIT)

# C++ library binding to Lua
set(SOL_INCLUDE_DIRS ${OpenMW_SOURCE_DIR}/extern/sol3.2.2 ${OpenMW_SOURCE_DIR}/extern/sol_config)

include_directories(
    BEFORE SYSTEM
    "."
    ${SDL2_INCLUDE_DIR}
    ${Boost_INCLUDE_DIR}
    ${MyGUI_INCLUDE_DIRS}
    ${OPENAL_INCLUDE_DIR}
    ${OPENGL_INCLUDE_DIR}
    ${BULLET_INCLUDE_DIRS}
    ${LUA_INCLUDE_DIR}
    ${SOL_INCLUDE_DIRS}
)

link_directories(${SDL2_LIBRARY_DIRS} ${Boost_LIBRARY_DIRS})

if(MYGUI_STATIC)
   add_definitions(-DMYGUI_STATIC)
endif (MYGUI_STATIC)

if (APPLE)
    configure_file(${OpenMW_SOURCE_DIR}/files/mac/openmw-Info.plist.in
        "${APP_BUNDLE_DIR}/Contents/Info.plist")

    configure_file(${OpenMW_SOURCE_DIR}/files/mac/openmw.icns
        "${APP_BUNDLE_DIR}/Contents/Resources/OpenMW.icns" COPYONLY)
endif (APPLE)

if (NOT APPLE)
    set(OPENMW_MYGUI_FILES_ROOT ${OpenMW_BINARY_DIR})
    set(OPENMW_SHADERS_ROOT ${OpenMW_BINARY_DIR})
endif ()

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

pack_resource_file(${OpenMW_SOURCE_DIR}/files/settings-default.cfg
    "${OpenMW_BINARY_DIR}" "defaults.bin")

configure_resource_file(${OpenMW_SOURCE_DIR}/files/openmw.appdata.xml
    "${OpenMW_BINARY_DIR}" "openmw.appdata.xml")

if (NOT APPLE)
    configure_resource_file(${OpenMW_SOURCE_DIR}/files/openmw.cfg.local
        "${OpenMW_BINARY_DIR}" "openmw.cfg")
    configure_resource_file(${OpenMW_SOURCE_DIR}/files/openmw.cfg
        "${OpenMW_BINARY_DIR}" "openmw.cfg.install")
else ()
    configure_file(${OpenMW_SOURCE_DIR}/files/openmw.cfg
        "${OpenMW_BINARY_DIR}/openmw.cfg")
endif ()

pack_resource_file(${OpenMW_SOURCE_DIR}/files/openmw-cs.cfg
    "${OpenMW_BINARY_DIR}" "defaults-cs.bin")

# Needs the copy version because the configure version assumes the end of the file has been reached when a null character is reached and there are no CMake expressions to evaluate.
copy_resource_file(${OpenMW_SOURCE_DIR}/files/opencs/defaultfilters
    "${OpenMW_BINARY_DIR}" "resources/defaultfilters")

configure_resource_file(${OpenMW_SOURCE_DIR}/files/gamecontrollerdb.txt
    "${OpenMW_BINARY_DIR}" "gamecontrollerdb.txt")

if (NOT WIN32 AND NOT APPLE)
    configure_file(${OpenMW_SOURCE_DIR}/files/org.openmw.launcher.desktop
        "${OpenMW_BINARY_DIR}/org.openmw.launcher.desktop")
    configure_file(${OpenMW_SOURCE_DIR}/files/openmw.appdata.xml
        "${OpenMW_BINARY_DIR}/openmw.appdata.xml")
    configure_file(${OpenMW_SOURCE_DIR}/files/org.openmw.cs.desktop
        "${OpenMW_BINARY_DIR}/org.openmw.cs.desktop")
endif()

if(OPENMW_LTO_BUILD)
    if(NOT CMAKE_VERSION VERSION_LESS 3.9)
        include(CheckIPOSupported)
        check_ipo_supported(RESULT HAVE_IPO OUTPUT HAVE_IPO_OUTPUT)
        if(HAVE_IPO)
            message(STATUS "LTO enabled for Release configuration.")
            set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
        else()
            message(WARNING "Requested option OPENMW_LTO_BUILD not supported by this compiler: ${HAVE_IPO_OUTPUT}")
            if(MSVC)
                message(STATUS "Note: Flags used to be set manually for this setting with MSVC. We now rely on CMake for this. Upgrade CMake to at least 3.13 to re-enable this setting.")
            endif()
        endif()
    else()
        message(WARNING "Requested option OPENMW_LTO_BUILD not supported by this cmake version: ${CMAKE_VERSION}. Upgrade CMake to at least 3.9 to enable support for certain compilers. Newer CMake versions support more compilers.")
    endif()
endif()


if (CMAKE_CXX_COMPILER_ID STREQUAL GNU OR CMAKE_CXX_COMPILER_ID STREQUAL Clang)
    set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wundef -Wno-unused-parameter -pedantic -Wno-long-long ${CMAKE_CXX_FLAGS}")
    add_definitions( -DBOOST_NO_CXX11_SCOPED_ENUMS=ON )

    if (APPLE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")
    endif()

    if (CMAKE_CXX_COMPILER_ID STREQUAL Clang AND NOT APPLE)
        if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 3.6 OR CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL 3.6)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-potentially-evaluated-expression")
        endif ()
    endif()

    if (CMAKE_CXX_COMPILER_ID STREQUAL GNU AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.6 OR CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL 4.6)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-but-set-parameter")
    endif()
endif (CMAKE_CXX_COMPILER_ID STREQUAL GNU OR CMAKE_CXX_COMPILER_ID STREQUAL Clang)

# Extern

add_subdirectory (extern/osg-ffmpeg-videoplayer)
add_subdirectory (extern/oics)
add_subdirectory (extern/Base64)
if (BUILD_OPENCS)
    add_subdirectory (extern/osgQt)
endif()

if (OPENMW_CXX_FLAGS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OPENMW_CXX_FLAGS}")
endif()

# Components
add_subdirectory (components)

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

if (BUILD_BENCHMARKS)
  add_subdirectory(apps/benchmarks)
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
      set_target_properties(openmw PROPERTIES COMPILE_DEFINITIONS $<$<CONFIG:Debug>:_CONSOLE>)
    elseif (BUILD_OPENMW)
      # Turn off debug console, debug output will be written to visual studio output instead
      set_target_properties(openmw PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:WINDOWS")
      set_target_properties(openmw PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:WINDOWS")
    endif()

    if (BUILD_OPENMW)
        # Release builds don't use the debug console
        set_target_properties(openmw PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")
        set_target_properties(openmw PROPERTIES LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:WINDOWS")
    endif()

    # Play a bit with the warning levels

    set(WARNINGS "/W4")

    set(WARNINGS_DISABLE
        4100 # Unreferenced formal parameter (-Wunused-parameter)
        4127 # Conditional expression is constant
        4996 # Function was declared deprecated
        )
    
    if( "${MyGUI_VERSION}" VERSION_LESS_EQUAL "3.4.0" )
        set(WARNINGS_DISABLE ${WARNINGS_DISABLE}
        4866 # compiler may not enforce left-to-right evaluation order for call
        )
    endif()

    if( "${MyGUI_VERSION}" VERSION_LESS_EQUAL "3.4.1" )
        set(WARNINGS_DISABLE ${WARNINGS_DISABLE}
        4275 # non dll-interface class 'MyGUI::delegates::IDelegateUnlink' used as base for dll-interface class 'MyGUI::Widget'
        )
    endif()

    foreach(d ${WARNINGS_DISABLE})
        set(WARNINGS "${WARNINGS} /wd${d}")
    endforeach(d)

    set_target_properties(components PROPERTIES COMPILE_FLAGS "${WARNINGS} ${MT_BUILD}")
    set_target_properties(osg-ffmpeg-videoplayer PROPERTIES COMPILE_FLAGS "${WARNINGS} ${MT_BUILD}")
    
    if (MSVC_VERSION GREATER_EQUAL 1915 AND MSVC_VERSION LESS 1920)
        target_compile_definitions(components INTERFACE _ENABLE_EXTENDED_ALIGNED_STORAGE)
    endif()

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
        # \bigobj is required:
        #  1) for OPENMW_UNITY_BUILD;
        #  2) to compile lua bindings, because sol3 is heavily templated.
        set_target_properties(openmw PROPERTIES COMPILE_FLAGS "${WARNINGS} ${MT_BUILD} /bigobj")
    endif()

    if (BUILD_WIZARD)
        set_target_properties(openmw-wizard PROPERTIES COMPILE_FLAGS "${WARNINGS} ${MT_BUILD}")
    endif()

    if (BUILD_BENCHMARKS)
        set_target_properties(openmw_detournavigator_navmeshtilescache_benchmark PROPERTIES COMPILE_FLAGS "${WARNINGS} ${MT_BUILD}")
    endif()
  endif(MSVC)

  # TODO: At some point release builds should not use the console but rather write to a log file
  #set_target_properties(openmw PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")
  #set_target_properties(openmw PROPERTIES LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:WINDOWS")
endif()

if (BUILD_OPENMW AND APPLE)
    if (USE_LUAJIT)
        # Without these flags LuaJit crashes on startup on OSX
        set_target_properties(openmw PROPERTIES LINK_FLAGS "-pagezero_size 10000 -image_base 100000000")
    endif(USE_LUAJIT)
    target_compile_definitions(components PRIVATE GL_SILENCE_DEPRECATION=1)
    target_compile_definitions(openmw PRIVATE GL_SILENCE_DEPRECATION=1)
endif()

# Apple bundling
if (OPENMW_OSX_DEPLOYMENT AND APPLE)
    if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.13 AND CMAKE_VERSION VERSION_LESS 3.13.4)
        message(FATAL_ERROR "macOS packaging is broken in early CMake 3.13 releases, see https://gitlab.com/OpenMW/openmw/issues/4767. Please use at least 3.13.4 or an older version like 3.12.4")
    endif ()

    get_property(QT_COCOA_PLUGIN_PATH TARGET Qt5::QCocoaIntegrationPlugin PROPERTY LOCATION_RELEASE)
    get_filename_component(QT_COCOA_PLUGIN_DIR "${QT_COCOA_PLUGIN_PATH}" DIRECTORY)
    get_filename_component(QT_COCOA_PLUGIN_GROUP "${QT_COCOA_PLUGIN_DIR}" NAME)
    get_filename_component(QT_COCOA_PLUGIN_NAME "${QT_COCOA_PLUGIN_PATH}" NAME)
    configure_file("${QT_COCOA_PLUGIN_PATH}" "${APP_BUNDLE_DIR}/Contents/PlugIns/${QT_COCOA_PLUGIN_GROUP}/${QT_COCOA_PLUGIN_NAME}" COPYONLY)
    configure_file("${OpenMW_SOURCE_DIR}/files/mac/qt.conf" "${APP_BUNDLE_DIR}/Contents/Resources/qt.conf" COPYONLY)

    if (BUILD_OPENCS)
      get_property(OPENCS_BUNDLE_NAME_TMP TARGET openmw-cs PROPERTY OUTPUT_NAME)
      set(OPENCS_BUNDLE_NAME "${OPENCS_BUNDLE_NAME_TMP}.app")
      configure_file("${QT_COCOA_PLUGIN_PATH}" "${OPENCS_BUNDLE_NAME}/Contents/PlugIns/${QT_COCOA_PLUGIN_GROUP}/${QT_COCOA_PLUGIN_NAME}" COPYONLY)
      configure_file("${OpenMW_SOURCE_DIR}/files/mac/qt.conf" "${OPENCS_BUNDLE_NAME}/Contents/Resources/qt.conf" COPYONLY)
    endif ()

    install(DIRECTORY "${APP_BUNDLE_DIR}" USE_SOURCE_PERMISSIONS DESTINATION "." COMPONENT Runtime)

    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_PACKAGE_VERSION ${OPENMW_VERSION})
    set(CPACK_PACKAGE_VERSION_MAJOR ${OPENMW_VERSION_MAJOR})
    set(CPACK_PACKAGE_VERSION_MINOR ${OPENMW_VERSION_MINOR})
    set(CPACK_PACKAGE_VERSION_PATCH ${OPENMW_VERSION_RELEASE})

    set(INSTALLED_OPENMW_APP "\${CMAKE_INSTALL_PREFIX}/${APP_BUNDLE_NAME}")
    set(INSTALLED_OPENCS_APP "\${CMAKE_INSTALL_PREFIX}/${OPENCS_BUNDLE_NAME}")

    install(CODE "
        set(BU_CHMOD_BUNDLE_ITEMS ON)
        set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH})
        include(BundleUtilities)
        cmake_minimum_required(VERSION 3.1)
    " COMPONENT Runtime)

    set(ABSOLUTE_PLUGINS "")

    set(OSGPlugins_DONT_FIND_DEPENDENCIES 1)
    find_package(OSGPlugins REQUIRED COMPONENTS ${USED_OSG_PLUGINS})

    foreach (PLUGIN_NAME ${USED_OSG_PLUGINS})
        string(TOUPPER ${PLUGIN_NAME} PLUGIN_NAME_UC)
        if(${PLUGIN_NAME_UC}_LIBRARY_RELEASE)
            set(PLUGIN_ABS ${${PLUGIN_NAME_UC}_LIBRARY_RELEASE})
        elseif(${PLUGIN_NAME_UC}_LIBRARY)
            set(PLUGIN_ABS ${${PLUGIN_NAME_UC}_LIBRARY})
        else()
            message(FATAL_ERROR "Can't find library file for ${PLUGIN_NAME}")
            # We used to construct the path manually from OSGPlugins_LIB_DIR and the plugin name.
            # Maybe that could be restored as a fallback?
        endif()
        set(ABSOLUTE_PLUGINS ${PLUGIN_ABS} ${ABSOLUTE_PLUGINS})
    endforeach ()

    set(OSG_PLUGIN_PREFIX_DIR "osgPlugins-${OPENSCENEGRAPH_VERSION}")

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

    install_plugins_for_bundle("${APP_BUNDLE_NAME}" PLUGINS)
    install_plugins_for_bundle("${OPENCS_BUNDLE_NAME}" OPENCS_PLUGINS)

    set(PLUGINS ${PLUGINS} "${INSTALLED_OPENMW_APP}/Contents/PlugIns/${QT_COCOA_PLUGIN_GROUP}/${QT_COCOA_PLUGIN_NAME}")
    set(OPENCS_PLUGINS ${OPENCS_PLUGINS} "${INSTALLED_OPENCS_APP}/Contents/PlugIns/${QT_COCOA_PLUGIN_GROUP}/${QT_COCOA_PLUGIN_NAME}")

    install(CODE "
        function(gp_item_default_embedded_path_override item  default_embedded_path_var)
            if (\${item} MATCHES ${OSG_PLUGIN_PREFIX_DIR})
              set(path \"@executable_path/../PlugIns/${OSG_PLUGIN_PREFIX_DIR}\")
              set(\${default_embedded_path_var} \"\${path}\" PARENT_SCOPE)
            endif()
        endfunction()

        fixup_bundle(\"${INSTALLED_OPENMW_APP}\" \"${PLUGINS}\" \"\")
        fixup_bundle(\"${INSTALLED_OPENCS_APP}\" \"${OPENCS_PLUGINS}\" \"\")
        " COMPONENT Runtime)
    include(CPack)
elseif(NOT APPLE)
    get_generator_is_multi_config(multi_config)
    if (multi_config)
        SET(INSTALL_SOURCE "${OpenMW_BINARY_DIR}/$<CONFIG>")
    else ()
        SET(INSTALL_SOURCE "${OpenMW_BINARY_DIR}")
    endif ()

    if(WIN32)
        INSTALL(DIRECTORY "${INSTALL_SOURCE}/" DESTINATION "." FILES_MATCHING PATTERN "*.dll"
                                                                              PATTERN "deps" EXCLUDE
                                                                              PATTERN "apps" EXCLUDE
                                                                              PATTERN "CMakeFiles" EXCLUDE
                                                                              PATTERN "components" EXCLUDE
                                                                              PATTERN "docs" EXCLUDE
                                                                              PATTERN "extern" EXCLUDE
                                                                              PATTERN "files" EXCLUDE
                                                                              PATTERN "Testing" EXCLUDE)
        INSTALL(DIRECTORY "${INSTALL_SOURCE}/" DESTINATION "." CONFIGURATIONS Debug;RelWithDebInfo FILES_MATCHING PATTERN "*.pdb"
                                                                                                                  PATTERN "deps" EXCLUDE
                                                                                                                  PATTERN "apps" EXCLUDE
                                                                                                                  PATTERN "CMakeFiles" EXCLUDE
                                                                                                                  PATTERN "components" EXCLUDE
                                                                                                                  PATTERN "docs" EXCLUDE
                                                                                                                  PATTERN "extern" EXCLUDE
                                                                                                                  PATTERN "files" EXCLUDE
                                                                                                                  PATTERN "Testing" EXCLUDE)
        INSTALL(FILES "${INSTALL_SOURCE}/openmw.cfg.install" DESTINATION "." RENAME "openmw.cfg")
        INSTALL(FILES "${OpenMW_SOURCE_DIR}/CHANGELOG.md" DESTINATION "." RENAME "CHANGELOG.txt")
        INSTALL(FILES "${OpenMW_SOURCE_DIR}/README.md" DESTINATION "." RENAME "README.txt")
        INSTALL(FILES "${OpenMW_SOURCE_DIR}/LICENSE" DESTINATION "." RENAME "LICENSE.txt")
        INSTALL(FILES "${OpenMW_SOURCE_DIR}/files/mygui/DejaVuFontLicense.txt" DESTINATION ".")
        INSTALL(FILES "${INSTALL_SOURCE}/defaults.bin" DESTINATION ".")
        INSTALL(FILES "${INSTALL_SOURCE}/gamecontrollerdb.txt" DESTINATION ".")

        INSTALL(DIRECTORY "${INSTALL_SOURCE}/resources" DESTINATION ".")

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
        SET(CPACK_NSIS_HELP_LINK "https:\\\\\\\\www.openmw.org")
        SET(CPACK_NSIS_URL_INFO_ABOUT "https:\\\\\\\\www.openmw.org")
        SET(CPACK_NSIS_INSTALLED_ICON_NAME "openmw-launcher.exe")
        SET(CPACK_NSIS_MUI_FINISHPAGE_RUN "openmw-launcher.exe")
        SET(CPACK_NSIS_MUI_ICON "${OpenMW_SOURCE_DIR}/files/windows/openmw.ico")
        SET(CPACK_NSIS_MUI_UNIICON "${OpenMW_SOURCE_DIR}/files/windows/openmw.ico")
        SET(CPACK_PACKAGE_ICON "${OpenMW_SOURCE_DIR}\\\\files\\\\openmw.bmp")

        SET(VCREDIST32 "${OpenMW_BINARY_DIR}/vcredist_x86.exe")
        if(EXISTS ${VCREDIST32})
            INSTALL(FILES ${VCREDIST32} DESTINATION "redist")
            SET(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "ExecWait '\\\"$INSTDIR\\\\redist\\\\vcredist_x86.exe\\\" /q /norestart'" )
        endif(EXISTS ${VCREDIST32})

        SET(VCREDIST64 "${OpenMW_BINARY_DIR}/vcredist_x64.exe")
        if(EXISTS ${VCREDIST64})
            INSTALL(FILES ${VCREDIST64} DESTINATION "redist")
            SET(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "ExecWait '\\\"$INSTDIR\\\\redist\\\\vcredist_x64.exe\\\" /q /norestart'" )
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
    else(WIN32)
        # Linux installation

        # Install binaries
        IF(BUILD_OPENMW)
            INSTALL(PROGRAMS "$<TARGET_FILE:openmw>" DESTINATION "${BINDIR}" )
        ENDIF(BUILD_OPENMW)
        IF(BUILD_LAUNCHER)
            INSTALL(PROGRAMS "${INSTALL_SOURCE}/openmw-launcher" DESTINATION "${BINDIR}" )
        ENDIF(BUILD_LAUNCHER)
        IF(BUILD_BSATOOL)
            INSTALL(PROGRAMS "${INSTALL_SOURCE}/bsatool" DESTINATION "${BINDIR}" )
        ENDIF(BUILD_BSATOOL)
        IF(BUILD_ESMTOOL)
            INSTALL(PROGRAMS "${INSTALL_SOURCE}/esmtool" DESTINATION "${BINDIR}" )
        ENDIF(BUILD_ESMTOOL)
        IF(BUILD_NIFTEST)
            INSTALL(PROGRAMS "${INSTALL_SOURCE}/niftest" DESTINATION "${BINDIR}" )
        ENDIF(BUILD_NIFTEST)
        IF(BUILD_MWINIIMPORTER)
            INSTALL(PROGRAMS "${INSTALL_SOURCE}/openmw-iniimporter" DESTINATION "${BINDIR}" )
        ENDIF(BUILD_MWINIIMPORTER)
        IF(BUILD_ESSIMPORTER)
            INSTALL(PROGRAMS "${INSTALL_SOURCE}/openmw-essimporter" DESTINATION "${BINDIR}" )
        ENDIF(BUILD_ESSIMPORTER)
        IF(BUILD_OPENCS)
            INSTALL(PROGRAMS "${INSTALL_SOURCE}/openmw-cs" DESTINATION "${BINDIR}" )
        ENDIF(BUILD_OPENCS)
        IF(BUILD_WIZARD)
            INSTALL(PROGRAMS "${INSTALL_SOURCE}/openmw-wizard" DESTINATION "${BINDIR}" )
        ENDIF(BUILD_WIZARD)

        # Install licenses
        INSTALL(FILES "files/mygui/DejaVuFontLicense.txt" DESTINATION "${LICDIR}" )

        # Install icon and desktop file
        INSTALL(FILES "${OpenMW_BINARY_DIR}/org.openmw.launcher.desktop" DESTINATION "${DATAROOTDIR}/applications" COMPONENT "openmw")
        INSTALL(FILES "${OpenMW_SOURCE_DIR}/files/launcher/images/openmw.png" DESTINATION "${ICONDIR}" COMPONENT "openmw")
        INSTALL(FILES "${OpenMW_BINARY_DIR}/openmw.appdata.xml" DESTINATION "${DATAROOTDIR}/metainfo" COMPONENT "openmw")
        IF(BUILD_OPENCS)
            INSTALL(FILES "${OpenMW_BINARY_DIR}/org.openmw.cs.desktop" DESTINATION "${DATAROOTDIR}/applications" COMPONENT "opencs")
            INSTALL(FILES "${OpenMW_SOURCE_DIR}/files/opencs/openmw-cs.png" DESTINATION "${ICONDIR}" COMPONENT "opencs")
        ENDIF(BUILD_OPENCS)

        # Install global configuration files
        INSTALL(FILES "${INSTALL_SOURCE}/defaults.bin" DESTINATION "${SYSCONFDIR}" COMPONENT "openmw")
        INSTALL(FILES "${INSTALL_SOURCE}/openmw.cfg.install" DESTINATION "${SYSCONFDIR}" RENAME "openmw.cfg" COMPONENT "openmw")
        INSTALL(FILES "${INSTALL_SOURCE}/resources/version" DESTINATION "${SYSCONFDIR}" COMPONENT "openmw")
        INSTALL(FILES "${INSTALL_SOURCE}/gamecontrollerdb.txt" DESTINATION "${SYSCONFDIR}" COMPONENT "openmw")

        IF(BUILD_OPENCS)
            INSTALL(FILES "${INSTALL_SOURCE}/defaults-cs.bin" DESTINATION "${SYSCONFDIR}" COMPONENT "opencs")
        ENDIF(BUILD_OPENCS)

        # Install resources
        INSTALL(DIRECTORY "${INSTALL_SOURCE}/resources" DESTINATION "${DATADIR}" COMPONENT "Resources")
        INSTALL(DIRECTORY DESTINATION "${DATADIR}/data" COMPONENT "Resources")
    endif(WIN32)
endif(OPENMW_OSX_DEPLOYMENT AND APPLE)

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
