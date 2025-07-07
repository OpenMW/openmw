cmake_minimum_required(VERSION 3.16.0)

#  set the timestamps of extracted contents to the time of extraction
# remove if cmake version is >= 3.24
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

project(OpenMW)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(GNUInstallDirs)

option(OPENMW_GL4ES_MANUAL_INIT "Manually initialize gl4es. This is more reliable on platforms without a windowing system. Requires gl4es to be configured with -DNOEGL=ON -DNO_LOADER=ON -DNO_INIT_CONSTRUCTOR=ON." OFF)
if(OPENMW_GL4ES_MANUAL_INIT)
    add_definitions(-DOPENMW_GL4ES_MANUAL_INIT)
endif()

if (APPLE OR WIN32)
    set(DEPLOY_QT_TRANSLATIONS_DEFAULT ON)
else ()
    set(DEPLOY_QT_TRANSLATIONS_DEFAULT OFF)
endif ()

option(DEPLOY_QT_TRANSLATIONS "Deploy standard Qt translations to resources folder. Needed when OpenMW applications are deployed with Qt libraries" ${DEPLOY_QT_TRANSLATIONS_DEFAULT})

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
option(BUILD_COMPONENTS_TESTS   "Build components library tests" OFF)
option(BUILD_BENCHMARKS         "Build benchmarks with Google Benchmark" OFF)
option(BUILD_NAVMESHTOOL        "Build navmesh tool" ON)
option(BUILD_BULLETOBJECTTOOL   "Build Bullet object tool" ON)
option(BUILD_OPENCS_TESTS       "Build OpenMW Construction Set tests" OFF)
option(BUILD_OPENMW_TESTS       "Build OpenMW tests" OFF)
option(PRECOMPILE_HEADERS_WITH_MSVC "Precompile most common used headers with MSVC (alternative to ccache)" ON)

set(OpenGL_GL_PREFERENCE LEGACY)  # Use LEGACY as we use GL2; GLNVD is for GL3 and up.

if (BUILD_LAUNCHER OR BUILD_OPENCS OR BUILD_WIZARD OR BUILD_OPENCS_TESTS)
    set(USE_QT TRUE)
else()
    set(USE_QT FALSE)
endif()

# If the user doesn't supply a CMAKE_BUILD_TYPE via command line, choose one for them.
IF(NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
        "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel."
        FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS None Debug Release RelWithDebInfo MinSizeRel)
ENDIF()

if (APPLE)
    set(CMAKE_FIND_FRAMEWORK LAST) # prefer dylibs over frameworks
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
set(OPENMW_VERSION_MINOR 50)
set(OPENMW_VERSION_RELEASE 0)
set(OPENMW_LUA_API_REVISION 80)
set(OPENMW_POSTPROCESSING_API_REVISION 3)

set(OPENMW_VERSION_COMMITHASH "")
set(OPENMW_VERSION_TAGHASH "")
set(OPENMW_VERSION_COMMITDATE "")

set(OPENMW_VERSION "${OPENMW_VERSION_MAJOR}.${OPENMW_VERSION_MINOR}.${OPENMW_VERSION_RELEASE}")

set(OPENMW_DOC_BASEURL "https://openmw.readthedocs.io/en/")

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

option(QT_STATIC "Link static build of Qt into the binaries" FALSE)

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
    find_package(RecastNavigation REQUIRED CONFIG)
else()
    set(_recastnavigation_static_default ON)
endif()
option(RECASTNAVIGATION_STATIC "Build recastnavigation static libraries" ${_recastnavigation_static_default})

option(OPENMW_USE_SYSTEM_SQLITE3 "Use system provided SQLite3 library" ON)

option(OPENMW_USE_SYSTEM_BENCHMARK "Use system Google Benchmark library." OFF)
option(OPENMW_USE_SYSTEM_GOOGLETEST "Use system Google Test library." OFF)

option(OPENMW_UNITY_BUILD "Use fewer compilation units to speed up compile time" FALSE)
option(OPENMW_LTO_BUILD "Build OpenMW with Link-Time Optimization (Needs ~2GB of RAM)" OFF)

# OS X deployment
option(OPENMW_OSX_DEPLOYMENT OFF)

if (MSVC)
    option(OPENMW_MP_BUILD "Build OpenMW with /MP flag" OFF)
    if (OPENMW_MP_BUILD)
        add_compile_options(/MP)
    endif()

    # \bigobj is required:
    #  1) for OPENMW_UNITY_BUILD;
    #  2) to compile lua bindings in components, openmw and tests, because sol3 is heavily templated.
    # there should be no relevant downsides to having it on:
    # https://docs.microsoft.com/en-us/cpp/build/reference/bigobj-increase-number-of-sections-in-dot-obj-file
    add_compile_options(/bigobj)

    add_compile_options(/Zc:__cplusplus)

    if (CMAKE_CXX_COMPILER_LAUNCHER OR CMAKE_C_COMPILER_LAUNCHER)
        if (CMAKE_GENERATOR MATCHES "Visual Studio")
            message(STATUS "A compiler launcher was specified, but will be unused by the current generator (${CMAKE_GENERATOR})")
        else()
            foreach (config_lower ${CMAKE_CONFIGURATION_TYPES})
                string(TOUPPER "${config_lower}" config)
                if (CMAKE_C_COMPILER_LAUNCHER STREQUAL "ccache")
                    string(REPLACE "/Zi" "/Z7" CMAKE_C_FLAGS_${config} "${CMAKE_C_FLAGS_${config}}")
                endif()
                if (CMAKE_CXX_COMPILER_LAUNCHER STREQUAL "ccache")
                    string(REPLACE "/Zi" "/Z7" CMAKE_CXX_FLAGS_${config} "${CMAKE_CXX_FLAGS_${config}}")
                endif()
            endforeach()
        endif()
    endif()
endif()

# Set up common paths
if (APPLE)
    set(OPENMW_RESOURCE_FILES "../Resources/resources" CACHE PATH "location of OpenMW resources files")
elseif(UNIX)
    # Paths
    SET(BINDIR "${CMAKE_INSTALL_PREFIX}/bin" CACHE PATH "Where to install binaries")
    SET(LIBDIR "${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}" CACHE PATH "Where to install libraries")
    SET(DATAROOTDIR "${CMAKE_INSTALL_DATAROOTDIR}" CACHE PATH "Sets the root of data directories to a non-default location")
    SET(GLOBAL_DATA_PATH "${CMAKE_INSTALL_PREFIX}/share/games/" CACHE PATH "Set data path prefix")
    SET(DATADIR "${GLOBAL_DATA_PATH}/openmw" CACHE PATH "Sets the openmw data directories to a non-default location")
    SET(ICONDIR "${DATAROOTDIR}/pixmaps" CACHE PATH "Set icon dir")
    SET(LICDIR "${DATAROOTDIR}/licenses/openmw" CACHE PATH "Sets the openmw license directory to a non-default location.")
    IF("${CMAKE_INSTALL_PREFIX}" STREQUAL "/usr")
        SET(GLOBAL_CONFIG_PATH "/etc/" CACHE PATH "Set config dir prefix")
    ELSE()
        SET(GLOBAL_CONFIG_PATH "${CMAKE_INSTALL_PREFIX}/etc/" CACHE PATH "Set config dir prefix")
    ENDIF()
    SET(SYSCONFDIR "${GLOBAL_CONFIG_PATH}/openmw" CACHE PATH "Set config dir")

    set(OPENMW_RESOURCE_FILES "${DATADIR}/resources" CACHE PATH "location of OpenMW resources files")
else()
    set(OPENMW_RESOURCE_FILES "resources" CACHE PATH "location of OpenMW resources files")
endif(APPLE)

if (WIN32)
    option(USE_DEBUG_CONSOLE "Whether a console should be displayed if OpenMW isn't launched from the command line. Does not affect the Release configuration." ON)
endif()

if(MSVC)
    add_compile_options("/utf-8")
endif()

# Dependencies
if (APPLE)
    # Force CMake to use the installed version of OpenGL on macOS
    set(_SAVE_CMAKE_FIND_FRAMEWORK ${CMAKE_FIND_FRAMEWORK})
    set(CMAKE_FIND_FRAMEWORK ONLY)
    find_package(OpenGL REQUIRED)
    set(CMAKE_FIND_FRAMEWORK ${_SAVE_CMAKE_FIND_FRAMEWORK})
    unset(_SAVE_CMAKE_FIND_FRAMEWORK)
else()
    find_package(OpenGL REQUIRED)
endif()

find_package(LZ4 REQUIRED)

if (USE_QT)
    find_package(QT REQUIRED COMPONENTS Core NAMES Qt6 Qt5)
    if (QT_VERSION_MAJOR VERSION_EQUAL 5)
        find_package(Qt5 5.15 COMPONENTS Core Widgets Network OpenGL LinguistTools Svg REQUIRED)
    else()
        find_package(Qt6 COMPONENTS Core Widgets Network OpenGL OpenGLWidgets LinguistTools Svg REQUIRED)
    endif()
    message(STATUS "Using Qt${QT_VERSION}")
endif()

set(USED_OSG_COMPONENTS
    osgAnimation
    osgDB
    osgGA
    osgFX
    osgParticle
    osgText
    osgUtil
    osgShadow
    osgSim
    osgViewer
    )
set(USED_OSG_PLUGINS
    osgdb_bmp
    osgdb_dae
    osgdb_dds
    osgdb_freetype
    osgdb_jpeg
    osgdb_osg
    osgdb_png
    osgdb_serializers_osg
    osgdb_tga)

if(NOT COLLADA_DOM_VERSION_MAJOR)
    set(COLLADA_DOM_VERSION_MAJOR 2)
endif()

if(NOT COLLADA_DOM_VERSION_MINOR)
    set(COLLADA_DOM_VERSION_MINOR 5)
endif()

find_package(collada_dom 2.5)

option(OPENMW_USE_SYSTEM_ICU "Use system ICU library instead of internal. If disabled, requires autotools" ON)
if(OPENMW_USE_SYSTEM_ICU)
    find_package(ICU REQUIRED COMPONENTS uc i18n data)
endif()

option(OPENMW_USE_SYSTEM_YAML_CPP "Use system yaml-cpp library instead of internal." ON)
if(OPENMW_USE_SYSTEM_YAML_CPP)
    set(_yaml_cpp_static_default OFF)
else()
    set(_yaml_cpp_static_default ON)
endif()
option(YAML_CPP_STATIC "Link static build of yaml-cpp into the binaries" ${_yaml_cpp_static_default})
if (OPENMW_USE_SYSTEM_YAML_CPP)
    find_package(yaml-cpp REQUIRED)
endif()

if ((BUILD_COMPONENTS_TESTS OR BUILD_OPENCS_TESTS OR BUILD_OPENMW_TESTS) AND OPENMW_USE_SYSTEM_GOOGLETEST)
    find_package(GTest 1.10 REQUIRED)
    find_package(GMock 1.10 REQUIRED)
endif()

add_subdirectory(extern)

# Sound setup

# Require at least ffmpeg 3.2 for now
SET(FFVER_OK FALSE)

find_package(FFmpeg REQUIRED COMPONENTS AVCODEC AVFORMAT AVUTIL SWSCALE SWRESAMPLE)

if(FFmpeg_FOUND)
    SET(FFVER_OK TRUE)

    # Can not detect FFmpeg version on Windows or Android for now
    if (NOT WIN32 AND NOT ANDROID)
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

# Required for unity build
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
    # Suppress WinMain(), provided by SDL
    add_definitions(-DSDL_MAIN_HANDLED)

    # Get rid of useless crud from windows.h
    add_definitions(
        -DNOMINMAX # name conflict with std::min, std::max
        -DWIN32_LEAN_AND_MEAN
        -DNOMB # name conflict with MWGui::MessageBox
        -DNOGDI # name conflict with osgAnimation::MorphGeometry::RELATIVE
    )
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
    find_package(OpenSceneGraph 3.6.5 REQUIRED ${USED_OSG_COMPONENTS})  # Bump to 3.6.6 when released

    if(OSG_STATIC)
        find_package(OSGPlugins REQUIRED COMPONENTS ${USED_OSG_PLUGINS})
    endif()
endif()

include_directories(BEFORE SYSTEM ${OPENSCENEGRAPH_INCLUDE_DIRS})

if(OSG_STATIC)
    add_definitions(-DOSG_LIBRARY_STATIC)
endif()

include(cmake/CheckOsgMultiview.cmake)
if(HAVE_MULTIVIEW)
    add_definitions(-DOSG_HAS_MULTIVIEW)
endif(HAVE_MULTIVIEW)

set(BOOST_COMPONENTS iostreams program_options system)

find_package(Boost 1.70.0 CONFIG REQUIRED COMPONENTS ${BOOST_COMPONENTS} OPTIONAL_COMPONENTS ${BOOST_OPTIONAL_COMPONENTS})

if(OPENMW_USE_SYSTEM_MYGUI)
    find_package(MyGUI 3.4.3 REQUIRED)
endif()
find_package(SDL2 2.0.10 REQUIRED)
find_package(OpenAL REQUIRED)
find_package(ZLIB REQUIRED)

option(USE_LUAJIT "Switch Lua/LuaJit (TRUE is highly recommended)" TRUE)
if(USE_LUAJIT)
    find_package(LuaJit REQUIRED)
    set(LUA_INCLUDE_DIR ${LuaJit_INCLUDE_DIR})
    set(LUA_LIBRARIES ${LuaJit_LIBRARIES})
else(USE_LUAJIT)
    find_package(Lua REQUIRED)
    add_compile_definitions(NO_LUAJIT)
endif(USE_LUAJIT)
if (NOT WIN32 AND NOT ANDROID)
    include(cmake/CheckLuaCustomAllocator.cmake)
endif()

# C++ library binding to Lua
set(SOL_INCLUDE_DIR ${OpenMW_SOURCE_DIR}/extern/sol3)
set(SOL_CONFIG_DIR ${OpenMW_SOURCE_DIR}/extern/sol_config)

include_directories(
    BEFORE SYSTEM
    "."
    ${MyGUI_INCLUDE_DIRS}
    ${OPENAL_INCLUDE_DIR}
    ${OPENGL_INCLUDE_DIR}
    ${BULLET_INCLUDE_DIRS}
    ${LUA_INCLUDE_DIR}
    ${SOL_INCLUDE_DIR}
    ${SOL_CONFIG_DIR}
    ${ICU_INCLUDE_DIRS}
)

link_directories(${COLLADA_DOM_LIBRARY_DIRS})

if(MYGUI_STATIC)
    add_definitions(-DMYGUI_STATIC)
endif (MYGUI_STATIC)

if (APPLE)
    configure_file(${OpenMW_SOURCE_DIR}/files/mac/openmw-Info.plist.in
        "${APP_BUNDLE_DIR}/Contents/Info.plist")

    configure_file(${OpenMW_SOURCE_DIR}/files/mac/openmw.icns
        "${APP_BUNDLE_DIR}/Contents/Resources/OpenMW.icns" COPYONLY)
endif (APPLE)

if (NOT APPLE)  # this is modified for macOS use later in "apps/open[mw|cs]/CMakeLists.txt"
    set(OPENMW_RESOURCES_ROOT ${OpenMW_BINARY_DIR})
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

if (APPLE)
    configure_file(${OpenMW_SOURCE_DIR}/files/openmw.cfg
        "${OpenMW_BINARY_DIR}/openmw.cfg")
elseif (WIN32)
    configure_resource_file(${OpenMW_SOURCE_DIR}/files/openmw.cfg.local
        "${OpenMW_BINARY_DIR}" "openmw.cfg")
    configure_resource_file(${OpenMW_SOURCE_DIR}/files/openmw.cfg.local
        "${OpenMW_BINARY_DIR}" "openmw.cfg.install")
else ()
    configure_resource_file(${OpenMW_SOURCE_DIR}/files/openmw.cfg.local
        "${OpenMW_BINARY_DIR}" "openmw.cfg")
    configure_resource_file(${OpenMW_SOURCE_DIR}/files/openmw.cfg
        "${OpenMW_BINARY_DIR}" "openmw.cfg.install")
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
endif()

if (APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")
endif()

# Extern

add_subdirectory (extern/osg-ffmpeg-videoplayer)
add_subdirectory (extern/oics)
add_subdirectory (extern/Base64)
if (BUILD_OPENCS OR BUILD_OPENCS_TESTS)
    add_subdirectory (extern/osgQt)
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    add_compile_options("/W4")

    set(WARNINGS_DISABLE
        4100 # Unreferenced formal parameter (-Wunused-parameter)
        4127 # Conditional expression is constant
        4996 # Function was declared deprecated
        5054 # Deprecated operations between enumerations of different types caused by Qt headers
        )

    foreach(d ${WARNINGS_DISABLE})
        add_compile_options("/wd${d}")
    endforeach(d)

    if(OPENMW_MSVC_WERROR)
        add_compile_options("/WX")
    endif()
else ()
    add_compile_options("-Wall" "-Wextra" "-Wundef" "-Wextra-semi" "-Wno-unused-parameter" "-pedantic" "-Wno-long-long" "-Wnon-virtual-dtor" "-Wunused")

    if (CMAKE_CXX_COMPILER_ID STREQUAL Clang AND NOT APPLE)
        if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 3.6 OR CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL 3.6)
            add_compile_options("-Wno-potentially-evaluated-expression")
        endif ()
    endif()

    if (CMAKE_CXX_COMPILER_ID STREQUAL GNU)
        add_compile_options("-Wno-unused-but-set-parameter" "-Wduplicated-branches" "-Wduplicated-cond" "-Wlogical-op")
        # https://gcc.gnu.org/bugzilla/show_bug.cgi?id=105438
        add_compile_options("-Wno-array-bounds")
    endif()
endif ()

if (OPENMW_CXX_FLAGS)
    separate_arguments(OPENMW_CXX_FLAGS NATIVE_COMMAND "${OPENMW_CXX_FLAGS}")
    add_compile_options(${OPENMW_CXX_FLAGS})
endif()

# Components
add_subdirectory (components)

# Apps and tools
if (BUILD_OPENMW OR BUILD_OPENMW_TESTS)
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

if (BUILD_OPENCS OR BUILD_OPENCS_TESTS)
    add_subdirectory (apps/opencs)
endif()

if (BUILD_WIZARD)
    add_subdirectory(apps/wizard)
endif()

if (BUILD_NIFTEST)
    add_subdirectory(apps/niftest)
endif()

if (BUILD_COMPONENTS_TESTS)
    add_subdirectory(apps/components_tests)
endif()

if (BUILD_BENCHMARKS)
    add_subdirectory(apps/benchmarks)
endif()

if (BUILD_NAVMESHTOOL)
    add_subdirectory(apps/navmeshtool)
endif()

if (BUILD_BULLETOBJECTTOOL)
    add_subdirectory(apps/bulletobjecttool)
endif()

if (BUILD_OPENCS_TESTS)
    add_subdirectory(apps/opencs_tests)
endif()

if (BUILD_OPENMW_TESTS)
    add_subdirectory(apps/openmw_tests)
endif()

if (WIN32)
    if (MSVC)
        foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
            string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
            set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "$(SolutionDir)$(Configuration)" )
            set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "$(ProjectDir)$(Configuration)" )
        endforeach( OUTPUTCONFIG )

        if (USE_DEBUG_CONSOLE AND BUILD_OPENMW)
            set_target_properties(openmw PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE")
            set_target_properties(openmw PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:CONSOLE")
        elseif (BUILD_OPENMW)
            # Turn off implicit console, you won't see stdout unless launching OpenMW from a command line shell or look at openmw.log
            set_target_properties(openmw PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:WINDOWS")
            set_target_properties(openmw PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:WINDOWS")
        endif()

        if (BUILD_OPENMW)
            # Release builds don't use the debug console
            set_target_properties(openmw PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")
            set_target_properties(openmw PROPERTIES LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:WINDOWS")
        endif()

        if (MSVC_VERSION GREATER_EQUAL 1915 AND MSVC_VERSION LESS 1920)
            target_compile_definitions(components INTERFACE _ENABLE_EXTENDED_ALIGNED_STORAGE)
        endif()
    endif(MSVC)

    # TODO: At some point release builds should not use the console but rather write to a log file
    #set_target_properties(openmw PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")
    #set_target_properties(openmw PROPERTIES LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:WINDOWS")
endif()

if (APPLE)
    target_compile_definitions(components PRIVATE GL_SILENCE_DEPRECATION=1)
    target_compile_definitions(openmw PRIVATE GL_SILENCE_DEPRECATION=1)
endif()

# Apple bundling
if (OPENMW_OSX_DEPLOYMENT AND APPLE)
    if (CMAKE_VERSION VERSION_LESS 3.19)
        message(FATAL_ERROR "macOS packaging requires CMake 3.19 or higher to sign macOS app bundles.")
    endif ()

    get_property(QT_COCOA_PLUGIN_PATH TARGET Qt${QT_VERSION_MAJOR}::QCocoaIntegrationPlugin PROPERTY LOCATION_RELEASE)
    get_filename_component(QT_COCOA_PLUGIN_DIR "${QT_COCOA_PLUGIN_PATH}" DIRECTORY)
    get_filename_component(QT_COCOA_PLUGIN_GROUP "${QT_COCOA_PLUGIN_DIR}" NAME)
    get_filename_component(QT_COCOA_PLUGIN_NAME "${QT_COCOA_PLUGIN_PATH}" NAME)
    configure_file("${QT_COCOA_PLUGIN_PATH}" "${APP_BUNDLE_DIR}/Contents/PlugIns/${QT_COCOA_PLUGIN_GROUP}/${QT_COCOA_PLUGIN_NAME}" COPYONLY)

    get_property(QT_QMACSTYLE_PLUGIN_PATH TARGET Qt${QT_VERSION_MAJOR}::QMacStylePlugin PROPERTY LOCATION_RELEASE)
    get_filename_component(QT_QMACSTYLE_PLUGIN_DIR "${QT_QMACSTYLE_PLUGIN_PATH}" DIRECTORY)
    get_filename_component(QT_QMACSTYLE_PLUGIN_GROUP "${QT_QMACSTYLE_PLUGIN_DIR}" NAME)
    get_filename_component(QT_QMACSTYLE_PLUGIN_NAME "${QT_QMACSTYLE_PLUGIN_PATH}" NAME)
    configure_file("${QT_QMACSTYLE_PLUGIN_PATH}" "${APP_BUNDLE_DIR}/Contents/PlugIns/${QT_QMACSTYLE_PLUGIN_GROUP}/${QT_QMACSTYLE_PLUGIN_NAME}" COPYONLY)

    get_property(QT_QSVG_PLUGIN_PATH TARGET Qt${QT_VERSION_MAJOR}::QSvgPlugin PROPERTY LOCATION_RELEASE)
    get_filename_component(QT_QSVG_PLUGIN_DIR "${QT_QSVG_PLUGIN_PATH}" DIRECTORY)
    get_filename_component(QT_QSVG_PLUGIN_GROUP "${QT_QSVG_PLUGIN_DIR}" NAME)
    get_filename_component(QT_QSVG_PLUGIN_NAME "${QT_QSVG_PLUGIN_PATH}" NAME)
    configure_file("${QT_QSVG_PLUGIN_PATH}" "${APP_BUNDLE_DIR}/Contents/PlugIns/${QT_QSVG_PLUGIN_GROUP}/${QT_QSVG_PLUGIN_NAME}" COPYONLY)

    get_property(QT_QSVG_ICON_PLUGIN_PATH TARGET Qt${QT_VERSION_MAJOR}::QSvgIconPlugin PROPERTY LOCATION_RELEASE)
    get_filename_component(QT_QSVG_ICON_PLUGIN_DIR "${QT_QSVG_ICON_PLUGIN_PATH}" DIRECTORY)
    get_filename_component(QT_QSVG_ICON_PLUGIN_GROUP "${QT_QSVG_ICON_PLUGIN_DIR}" NAME)
    get_filename_component(QT_QSVG_ICON_PLUGIN_NAME "${QT_QSVG_ICON_PLUGIN_PATH}" NAME)
    configure_file("${QT_QSVG_ICON_PLUGIN_PATH}" "${APP_BUNDLE_DIR}/Contents/PlugIns/${QT_QSVG_ICON_PLUGIN_GROUP}/${QT_QSVG_ICON_PLUGIN_NAME}" COPYONLY)

    configure_file("${OpenMW_SOURCE_DIR}/files/mac/qt.conf" "${APP_BUNDLE_DIR}/Contents/Resources/qt.conf" COPYONLY)

    if (BUILD_OPENCS)
        get_property(OPENCS_BUNDLE_NAME_TMP TARGET openmw-cs PROPERTY OUTPUT_NAME)
        set(OPENCS_BUNDLE_NAME "${OPENCS_BUNDLE_NAME_TMP}.app")
        configure_file("${QT_COCOA_PLUGIN_PATH}" "${OPENCS_BUNDLE_NAME}/Contents/PlugIns/${QT_COCOA_PLUGIN_GROUP}/${QT_COCOA_PLUGIN_NAME}" COPYONLY)
        configure_file("${QT_QMACSTYLE_PLUGIN_PATH}" "${OPENCS_BUNDLE_NAME}/Contents/PlugIns/${QT_QMACSTYLE_PLUGIN_GROUP}/${QT_QMACSTYLE_PLUGIN_NAME}" COPYONLY)
        configure_file("${QT_QSVG_PLUGIN_PATH}" "${OPENCS_BUNDLE_NAME}/Contents/PlugIns/${QT_QSVG_PLUGIN_GROUP}/${QT_QSVG_PLUGIN_NAME}" COPYONLY)
        configure_file("${QT_QSVG_ICON_PLUGIN_PATH}" "${OPENCS_BUNDLE_NAME}/Contents/PlugIns/${QT_QSVG_ICON_PLUGIN_GROUP}/${QT_QSVG_ICON_PLUGIN_NAME}" COPYONLY)
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

    INSTALL(FILES "${OpenMW_SOURCE_DIR}/CHANGELOG.md" DESTINATION "/${INSTALLED_OPENMW_APP}/..")
    INSTALL(FILES "${OpenMW_SOURCE_DIR}/README.md" DESTINATION "/${INSTALLED_OPENMW_APP}/..")
    INSTALL(FILES "${OpenMW_SOURCE_DIR}/LICENSE" DESTINATION "/${INSTALLED_OPENMW_APP}/..")
    INSTALL(FILES "${OpenMW_SOURCE_DIR}/AUTHORS.md" DESTINATION "/${INSTALLED_OPENMW_APP}/..")

    set(PLUGINS ${PLUGINS} "${INSTALLED_OPENMW_APP}/Contents/PlugIns/${QT_COCOA_PLUGIN_GROUP}/${QT_COCOA_PLUGIN_NAME}")
    set(PLUGINS ${PLUGINS} "${INSTALLED_OPENMW_APP}/Contents/PlugIns/${QT_QMACSTYLE_PLUGIN_GROUP}/${QT_QMACSTYLE_PLUGIN_NAME}")
    set(OPENCS_PLUGINS ${OPENCS_PLUGINS} "${INSTALLED_OPENCS_APP}/Contents/PlugIns/${QT_COCOA_PLUGIN_GROUP}/${QT_COCOA_PLUGIN_NAME}")
    set(OPENCS_PLUGINS ${OPENCS_PLUGINS} "${INSTALLED_OPENCS_APP}/Contents/PlugIns/${QT_QMACSTYLE_PLUGIN_GROUP}/${QT_QMACSTYLE_PLUGIN_NAME}")

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

    set(CPACK_PRE_BUILD_SCRIPTS ${CMAKE_SOURCE_DIR}/cmake/SignMacApplications.cmake)

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
                                                                              PATTERN ".cmake" EXCLUDE
                                                                              PATTERN ".qt" EXCLUDE
                                                                              PATTERN "_CPack_Packages" EXCLUDE
                                                                              PATTERN "_deps" EXCLUDE
                                                                              PATTERN "apps" EXCLUDE
                                                                              PATTERN "bin" EXCLUDE
                                                                              PATTERN "CMakeFiles" EXCLUDE
                                                                              PATTERN "components" EXCLUDE
                                                                              PATTERN "deps" EXCLUDE
                                                                              PATTERN "docs" EXCLUDE
                                                                              PATTERN "extern" EXCLUDE
                                                                              PATTERN "files" EXCLUDE
                                                                              PATTERN "lib" EXCLUDE
                                                                              PATTERN "SymStore" EXCLUDE
                                                                              PATTERN "symstore-venv" EXCLUDE
                                                                              PATTERN "Testing" EXCLUDE
                                                                              PATTERN "tests_output" EXCLUDE
                                                                              PATTERN "try-compile")
        INSTALL(DIRECTORY "${INSTALL_SOURCE}/" DESTINATION "." CONFIGURATIONS Debug;RelWithDebInfo FILES_MATCHING PATTERN "*.pdb"
                                                                                                                  PATTERN ".cmake" EXCLUDE
                                                                                                                  PATTERN ".qt" EXCLUDE
                                                                                                                  PATTERN "_CPack_Packages" EXCLUDE
                                                                                                                  PATTERN "_deps" EXCLUDE
                                                                                                                  PATTERN "apps" EXCLUDE
                                                                                                                  PATTERN "bin" EXCLUDE
                                                                                                                  PATTERN "CMakeFiles" EXCLUDE
                                                                                                                  PATTERN "components" EXCLUDE
                                                                                                                  PATTERN "deps" EXCLUDE
                                                                                                                  PATTERN "docs" EXCLUDE
                                                                                                                  PATTERN "extern" EXCLUDE
                                                                                                                  PATTERN "files" EXCLUDE
                                                                                                                  PATTERN "lib" EXCLUDE
                                                                                                                  PATTERN "SymStore" EXCLUDE
                                                                                                                  PATTERN "symstore-venv" EXCLUDE
                                                                                                                  PATTERN "Testing" EXCLUDE
                                                                                                                  PATTERN "tests_output" EXCLUDE
                                                                                                                  PATTERN "try-compile")
        INSTALL(FILES "${INSTALL_SOURCE}/openmw.cfg.install" DESTINATION "." RENAME "openmw.cfg")
        INSTALL(FILES "${OpenMW_SOURCE_DIR}/CHANGELOG.md" DESTINATION "." RENAME "CHANGELOG.txt")
        INSTALL(FILES "${OpenMW_SOURCE_DIR}/README.md" DESTINATION "." RENAME "README.txt")
        INSTALL(FILES "${OpenMW_SOURCE_DIR}/LICENSE" DESTINATION "." RENAME "LICENSE.txt")
        INSTALL(FILES "${OpenMW_SOURCE_DIR}/AUTHORS.md" DESTINATION "." RENAME "AUTHORS.txt")
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

        SET(VCREDIST64 "${OpenMW_BINARY_DIR}/vcredist_x64.exe" CACHE FILEPATH "Path to vcredist_x64.exe")
        if(EXISTS ${VCREDIST64})
            INSTALL(FILES ${VCREDIST64} DESTINATION "redist")
            get_filename_component(REDIST_FILENAME "${VCREDIST64}" NAME)
            SET(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "
                ExecWait '\\\"$INSTDIR\\\\redist\\\\${REDIST_FILENAME}\\\" /q /norestart'
                RMDir /r \\\"$INSTDIR\\\\redist\\\"
                ")
        endif(EXISTS ${VCREDIST64})

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
        if(BUILD_NAVMESHTOOL)
            install(PROGRAMS "${INSTALL_SOURCE}/openmw-navmeshtool" DESTINATION "${BINDIR}" )
        endif()
        IF(BUILD_BULLETOBJECTTOOL)
            INSTALL(PROGRAMS "${INSTALL_SOURCE}/openmw-bulletobjecttool" DESTINATION "${BINDIR}" )
        ENDIF(BUILD_BULLETOBJECTTOOL)

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
        INSTALL(FILES "${INSTALL_SOURCE}/gamecontrollerdb.txt" DESTINATION "${SYSCONFDIR}" COMPONENT "openmw")

        IF(BUILD_OPENCS)
            INSTALL(FILES "${INSTALL_SOURCE}/defaults-cs.bin" DESTINATION "${SYSCONFDIR}" COMPONENT "opencs")
        ENDIF(BUILD_OPENCS)

        # Install resources
        INSTALL(DIRECTORY "${INSTALL_SOURCE}/resources" DESTINATION "${DATADIR}" COMPONENT "Resources")
    endif(WIN32)
endif(OPENMW_OSX_DEPLOYMENT AND APPLE)

# what is necessary to build documentation
if ( BUILD_DOCS )
    # Builds the documentation.
    FIND_PACKAGE( Sphinx REQUIRED )
    FIND_PACKAGE( Doxygen REQUIRED )

    # Doxygen Target -- simply run 'make doc' or 'make doc_pages'
    # output directory for 'make doc'       is "${OpenMW_BINARY_DIR}/docs/Doxygen"
    # output directory for 'make doc_pages' is "${DOXYGEN_PAGES_OUTPUT_DIR}" if defined
    #                                       or "${OpenMW_BINARY_DIR}/docs/Pages" otherwise

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

# Qt localization
if (USE_QT)
    file(GLOB LAUNCHER_TS_FILES ${CMAKE_SOURCE_DIR}/files/lang/launcher_*.ts)
    file(GLOB WIZARD_TS_FILES ${CMAKE_SOURCE_DIR}/files/lang/wizard_*.ts)
    file(GLOB COMPONENTS_TS_FILES ${CMAKE_SOURCE_DIR}/files/lang/components_*.ts)
    get_target_property(QT_LUPDATE_EXECUTABLE Qt::lupdate IMPORTED_LOCATION)
    add_custom_target(translations
    COMMAND ${QT_LUPDATE_EXECUTABLE} -locations none ${CMAKE_SOURCE_DIR}/components/contentselector ${CMAKE_SOURCE_DIR}/components/process -ts ${COMPONENTS_TS_FILES}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/components
    VERBATIM
    COMMAND_EXPAND_LISTS

    COMMAND ${QT_LUPDATE_EXECUTABLE} -locations none ${CMAKE_SOURCE_DIR}/apps/wizard -ts ${WIZARD_TS_FILES}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/apps/wizard
    VERBATIM
    COMMAND_EXPAND_LISTS

    COMMAND ${QT_LUPDATE_EXECUTABLE} -locations none ${CMAKE_SOURCE_DIR}/apps/launcher -ts ${LAUNCHER_TS_FILES}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/apps/launcher
    VERBATIM
    COMMAND_EXPAND_LISTS)

    if (BUILD_LAUNCHER OR BUILD_WIZARD)
        if (APPLE)
            set(QT_OPENMW_TRANSLATIONS_PATH "${APP_BUNDLE_DIR}/Contents/Resources/resources/translations")
        else ()
            get_generator_is_multi_config(multi_config)
            if (multi_config)
                set(QT_OPENMW_TRANSLATIONS_PATH "${OpenMW_BINARY_DIR}/$<CONFIG>/resources/translations")
            else ()
                set(QT_OPENMW_TRANSLATIONS_PATH "${OpenMW_BINARY_DIR}/resources/translations")
            endif ()
        endif ()

        file(GLOB TS_FILES ${COMPONENTS_TS_FILES})

        if (BUILD_LAUNCHER)
            set(TS_FILES ${TS_FILES} ${LAUNCHER_TS_FILES})
        endif ()

        if (BUILD_WIZARD)
            set(TS_FILES ${TS_FILES} ${WIZARD_TS_FILES})
        endif ()

        qt_add_translation(QM_FILES ${TS_FILES} OPTIONS -silent)

        if (DEPLOY_QT_TRANSLATIONS)
            # Once we set a Qt 6.2.0 as a minimum required version, we may use "qtpaths --qt-query" instead.
            get_target_property(QT_QMAKE_EXECUTABLE Qt::qmake IMPORTED_LOCATION)
            execute_process(COMMAND "${QT_QMAKE_EXECUTABLE}" -query QT_INSTALL_TRANSLATIONS
                OUTPUT_VARIABLE QT_TRANSLATIONS_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)

            foreach(QM_FILE ${QM_FILES})
                get_filename_component(QM_BASENAME ${QM_FILE} NAME)
                string(REGEX REPLACE "[^_]+_(.*)\\.qm" "\\1" LANG_NAME ${QM_BASENAME})
                if (EXISTS "${QT_TRANSLATIONS_DIR}/qtbase_${LANG_NAME}.qm")
                    set(QM_FILES ${QM_FILES} "${QT_TRANSLATIONS_DIR}/qtbase_${LANG_NAME}.qm")
                elseif (EXISTS "${QT_TRANSLATIONS_DIR}/qt_${LANG_NAME}.qm")
                    set(QM_FILES ${QM_FILES} "${QT_TRANSLATIONS_DIR}/qt_${LANG_NAME}.qm")
                else ()
                    message(FATAL_ERROR "Qt translations for '${LANG_NAME}' locale are not found in the '${QT_TRANSLATIONS_DIR}' folder.")
                endif ()
            endforeach(QM_FILE)

            list(REMOVE_DUPLICATES QM_FILES)
        endif ()

        add_custom_target(qm-files
            COMMAND ${CMAKE_COMMAND} -E make_directory ${QT_OPENMW_TRANSLATIONS_PATH}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${QM_FILES} ${QT_OPENMW_TRANSLATIONS_PATH}
            DEPENDS ${QM_FILES}
            COMMENT "Copy *.qm files to resources folder")
    endif ()
endif()
