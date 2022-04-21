# SPDX-License-Identifier: GPL-3.0-or-later

set(CMAKE_CXX_CLANG_TIDY "")

# Like `FetchContent_MakeAvailable` but passes EXCLUDE_FROM_ALL to `add_subdirectory`.
macro(FetchContent_MakeAvailableExcludeFromAll)
    foreach(contentName IN ITEMS ${ARGV})
        string(TOLOWER ${contentName} contentNameLower)
        FetchContent_GetProperties(${contentName})
        if(NOT ${contentNameLower}_POPULATED)
            FetchContent_Populate(${contentName})
            if(EXISTS ${${contentNameLower}_SOURCE_DIR}/CMakeLists.txt)
                add_subdirectory(${${contentNameLower}_SOURCE_DIR}
                    ${${contentNameLower}_BINARY_DIR} EXCLUDE_FROM_ALL)
            endif()
        endif()
    endforeach()
endmacro()

if(NOT OPENMW_USE_SYSTEM_BULLET)
    cmake_minimum_required(VERSION 3.11) # for FetchContent

    set(BUILD_BULLET3 OFF CACHE BOOL "")
    set(BUILD_EXTRAS OFF CACHE BOOL "")
    set(BUILD_OPENGL3_DEMOS OFF CACHE BOOL "")
    set(BUILD_UNIT_TESTS OFF CACHE BOOL "")
    set(BUILD_BULLET2_DEMOS OFF CACHE BOOL "")
    set(BUILD_CLSOCKET OFF CACHE BOOL "")
    set(BUILD_ENET OFF CACHE BOOL "")
    set(BUILD_CPU_DEMOS OFF CACHE BOOL "")
    set(BUILD_EGL OFF CACHE BOOL "")

    set(USE_DOUBLE_PRECISION ON CACHE BOOL "")
    set(BULLET2_MULTITHREADING ON CACHE BOOL "")

    if(BULLET_STATIC)
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    else()
        set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
    endif()

    if(MSVC)
        # this setting is badly named - having it off forces the static runtime library,
        # but having it on does nothing, letting the defaults get used.
        # OpenMW uses the defaults, and you can't mix and match.
        set(USE_MSVC_RUNTIME_LIBRARY_DLL ON CACHE BOOL "" FORCE)
    endif()

    # May 7, 2021
    include(FetchContent)
    FetchContent_Declare(bullet
        URL https://github.com/bulletphysics/bullet3/archive/refs/tags/3.17.tar.gz
        URL_HASH SHA512=a5105bf5f1dd365a64a350755c7d2c97942f74897a18dcdb3651e6732fd55cc1030a096f5808cf50575281f05e3ac09aa50a48d271a47b94cd61f5167a72b7cc
        SOURCE_DIR fetched/bullet
    )
    FetchContent_MakeAvailableExcludeFromAll(bullet)

    set(BULLET_INCLUDE_DIRS ${bullet_SOURCE_DIR}/src PARENT_SCOPE)

    # The order here is important to work around a bug in Bullet:
    # https://github.com/bulletphysics/bullet3/issues/3233
    set(BULLET_LIBRARIES BulletCollision LinearMath PARENT_SCOPE)
endif()

if(NOT OPENMW_USE_SYSTEM_MYGUI)
    cmake_minimum_required(VERSION 3.11) # for FetchContent

    set(MYGUI_RENDERSYSTEM 4 CACHE STRING "")
    set(MYGUI_DISABLE_PLUGINS TRUE CACHE BOOL "")
    set(MYGUI_BUILD_DEMOS OFF CACHE BOOL "")
    set(MYGUI_BUILD_PLUGINS OFF CACHE BOOL "")
    set(MYGUI_BUILD_TOOLS OFF CACHE BOOL "")

    # We appear to be using some obsolete properties in the XML.
    # See https://gitlab.com/OpenMW/openmw/-/issues/5896
    set(MYGUI_DONT_USE_OBSOLETE OFF CACHE BOOL "")

    if(MYGUI_STATIC)
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    else()
        set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
    endif()

    # master on 13 Mar 2021
    include(FetchContent)
    FetchContent_Declare(mygui
        URL https://github.com/MyGUI/mygui/archive/59c1388b942721887d18743ada15f1906ff11a1f.zip
        URL_HASH SHA512=56b112b261fdc4c9b782de8ef8a9d239af3a75b8cfaa617daecc77c38ecd6521624cf5c17e6ac839b0870be5c31608d4f2a1574165d9d049f14f9e6ae03a8f98
        SOURCE_DIR fetched/mygui
    )
    FetchContent_MakeAvailableExcludeFromAll(mygui)

    set(MyGUI_INCLUDE_DIRS ${mygui_SOURCE_DIR}/MyGUIEngine/include PARENT_SCOPE)
    set(MyGUI_LIBRARIES MyGUIEngine PARENT_SCOPE)
endif()

if(NOT OPENMW_USE_SYSTEM_OSG)
    cmake_minimum_required(VERSION 3.11) # for FetchContent

    set(BUILD_OSG_APPLICATIONS OFF CACHE BOOL "")
    set(BUILD_OSG_DEPRECATED_SERIALIZERS OFF CACHE BOOL "")
    set(OSG_FIND_3RD_PARTY_DEPS OFF CACHE BOOL "")

    set(BUILD_OSG_PLUGINS_BY_DEFAULT OFF CACHE BOOL "")
    set(BUILD_OSG_PLUGIN_BMP ON CACHE BOOL "")
    set(BUILD_OSG_PLUGIN_DDS ON CACHE BOOL "")
    set(BUILD_OSG_PLUGIN_FREETYPE ON CACHE BOOL "")
    set(BUILD_OSG_PLUGIN_JPEG ON CACHE BOOL "")
    set(BUILD_OSG_PLUGIN_OSG ON CACHE BOOL "")
    set(BUILD_OSG_PLUGIN_PNG ON CACHE BOOL "")
    set(BUILD_OSG_PLUGIN_TGA ON CACHE BOOL "")
    set(BUILD_OSG_PLUGIN_KTX ON CACHE BOOL "")

    set(OSG_USE_FLOAT_MATRIX ON CACHE BOOL "")
    set(OSG_USE_FLOAT_PLANE ON CACHE BOOL "")
    set(OSG_USE_FLOAT_QUAT ON CACHE BOOL "")

    set(OPENGL_PROFILE "GL2" CACHE STRING "")

    if(OSG_STATIC)
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
        set(DYNAMIC_OPENTHREADS OFF CACHE BOOL "" FORCE)
        set(DYNAMIC_OPENSCENEGRAPH OFF CACHE BOOL "" FORCE)
    else()
        set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
        set(DYNAMIC_OPENTHREADS ON CACHE BOOL "" FORCE)
        set(DYNAMIC_OPENSCENEGRAPH ON CACHE BOOL "" FORCE)
    endif()
    mark_as_advanced(DYNAMIC_OPENTHREADS DYNAMIC_OPENSCENEGRAPH)

    if(WIN32)
        # OSG here inherits C++17 language level because it doesn't specify its own.
        #
        # OSG's `using namespace std` interferes with Windows header files.
        #
        # See https://developercommunity.visualstudio.com/t/error-c2872-byte-ambiguous-symbol/93889
        #
        # An alternative way to work around this without changing the language level is:
        #
        # add_compile_definitions(_HAS_STD_BYTE=0)
        #
        # TODO: Put OSG into its own scope so that this does not leak into Recast below.
        set(CMAKE_CXX_STANDARD 11)

        if(MSVC)
            set(OSG_MSVC_VERSIONED_DLL OFF CACHE BOOL "")
        endif()
    endif()

    # OSGoS branch 3.6
    include(FetchContent)
    FetchContent_Declare(osg
        URL https://github.com/OpenMW/osg/archive/01cc2b585c8456a4ff843066b7e1a8715558289f.zip
        URL_HASH SHA512=9b0a94c1c1d99c767f1857629d43c7a53bfcb74ef760993a121567831e168a1ebbfc10b0c67d7f2241c7b6c6dab2d0e6b876d9f17aca0fabe1a8c86b2279f956
        SOURCE_DIR fetched/osg
    )
    FetchContent_MakeAvailableExcludeFromAll(osg)

    set(OPENSCENEGRAPH_INCLUDE_DIRS ${osg_SOURCE_DIR}/include ${osg_BINARY_DIR}/include PARENT_SCOPE)
    set(OSG_LIBRARIES OpenThreads osg PARENT_SCOPE)
    foreach(_name ${USED_OSG_COMPONENTS})
        string(TOUPPER ${_name} _name_uc)
        set(${_name_uc}_LIBRARIES ${_name} PARENT_SCOPE)
    endforeach()
    foreach(_name ${USED_OSG_PLUGINS})
        string(TOUPPER ${_name} _name_uc)
        set(${_name_uc}_LIBRARY ${_name} PARENT_SCOPE)
    endforeach()
endif()

if(NOT OPENMW_USE_SYSTEM_RECASTNAVIGATION)
    if(RECASTNAVIGATION_STATIC)
        set(BUILD_SHARED_LIBS OFF)
    else()
        set(BUILD_SHARED_LIBS ON)
    endif()

    set(RECASTNAVIGATION_DEMO OFF CACHE BOOL "")
    set(RECASTNAVIGATION_TESTS OFF CACHE BOOL "")
    set(RECASTNAVIGATION_EXAMPLES OFF CACHE BOOL "")

    # master on 15 Feb 2021
    include(FetchContent)
    FetchContent_Declare(recastnavigation
        URL https://github.com/recastnavigation/recastnavigation/archive/e75adf86f91eb3082220085e42dda62679f9a3ea.zip
        URL_HASH SHA512=93a19490cdfa55e98a6af9cc050e94af88fdb95fae2059ceeff28b62f3b48515f5fdd2c806c910550933b6861a4f6a91173ee0ed1b61c1396f7b34d4c78f0793
        SOURCE_DIR fetched/recastnavigation
    )
    FetchContent_MakeAvailableExcludeFromAll(recastnavigation)
endif()

if (NOT OPENMW_USE_SYSTEM_SQLITE3)
    include(FetchContent)
    FetchContent_Declare(sqlite3
        URL https://www.sqlite.org/2021/sqlite-amalgamation-3360000.zip
        URL_HASH SHA512=5c18f158a599b1e91d95c91de3aa5c5de52f986845ad0cb49dfd56b650587e55e24d469571b5b864229b870d0eaf85d78893f61ef950b95389cb41692be37f58
        SOURCE_DIR fetched/sqlite3
    )
    FetchContent_MakeAvailableExcludeFromAll(sqlite3)

    add_library(sqlite3 STATIC ${sqlite3_SOURCE_DIR}/sqlite3.c)
    target_include_directories(sqlite3 INTERFACE ${sqlite3_SOURCE_DIR}/)
    if (UNIX)
        target_link_libraries(sqlite3 ${CMAKE_DL_LIBS})
    endif()
    add_library(SQLite::SQLite3 ALIAS sqlite3)

    set(SQLite3_INCLUDE_DIR ${sqlite3_SOURCE_DIR}/ PARENT_SCOPE)
    set(SQLite3_LIBRARY sqlite3 PARENT_SCOPE)
endif()

add_subdirectory(smhasher)

if (BUILD_BENCHMARKS AND NOT OPENMW_USE_SYSTEM_BENCHMARK)
    cmake_minimum_required(VERSION 3.11)

    set(BENCHMARK_ENABLE_TESTING OFF)
    set(BENCHMARK_ENABLE_INSTALL OFF)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF)

    include(FetchContent)
    FetchContent_Declare(benchmark
        URL https://github.com/google/benchmark/archive/refs/tags/v1.5.2.zip
        URL_HASH SHA512=1146deca3b424703800933012ef75805d4657309d58b3484498bb51a99025bd405a69855a75af59c23fc3684bfa027224513999b8d7beaab3320c96fb6d6c540
        SOURCE_DIR fetched/benchmark
    )
    FetchContent_MakeAvailableExcludeFromAll(benchmark)
endif()

if (NOT OPENMW_USE_SYSTEM_YAML_CPP)
    if(YAML_CPP_STATIC)
        set(YAML_BUILD_SHARED_LIBS OFF)
    else()
        set(YAML_BUILD_SHARED_LIBS ON)
    endif()

    include(FetchContent)
    FetchContent_Declare(yaml-cpp
        URL https://github.com/jbeder/yaml-cpp/archive/refs/tags/yaml-cpp-0.7.0.zip
        URL_HASH MD5=1e8ca0d6ccf99f3ed9506c1f6937d0ec
        SOURCE_DIR fetched/yaml-cpp
    )
    FetchContent_MakeAvailableExcludeFromAll(yaml-cpp)
endif()

if (NOT OPENMW_USE_SYSTEM_ICU)
    set(ICU_ENV "ICU_DATA_FILTER_FILE=${CMAKE_CURRENT_SOURCE_DIR}/icufilters.json")
    if (ANDROID)
        # Note: Must be a build directory, not an install root, since the configure script
        # looks for a configuration file which does not get installed.
        set(OPENMW_ICU_HOST_BUILD_DIR "" CACHE STRING "A pre-built ICU build directory for the host system if cross-compiling")
        if (OPENMW_ICU_HOST_BUILD_DIR STREQUAL "")
            message(FATAL_ERROR "If cross-compiling on android you must set the \
            OPENMW_ICU_HOST_BUILD_DIR to the path of a pre-compiled build of \
            ICU 70.1 for the system doing the build, as ICU needs to be able \
            to run its own executables as part of the build process.")
        endif()
        # We need a host version of ICU so that the tools can be run when building the data library.
        set(NDK_STANDARD_ROOT ${CMAKE_ANDROID_NDK}/toolchains/llvm/prebuilt/linux-x86_64)
        string(REPLACE "android-" "" ANDROIDVER ${ANDROID_PLATFORM})
        # Wants a triple such as aarch64-linux-android, excluding a trailing
        # -clang etc.
        string(REGEX MATCH "^[^-]\+-[^-]+-[^-]+" ICU_TOOLCHAIN_NAME ${ANDROID_TOOLCHAIN_NAME})
        set(ICU_ENV
            ${ICU_ENV}
            "CC=${CMAKE_C_COMPILER_LAUNCHER} ${NDK_STANDARD_ROOT}/bin/${ICU_TOOLCHAIN_NAME}${ANDROIDVER}-clang"
            "CXX=${CMAKE_CXX_COMPILER_LAUNCHER} ${NDK_STANDARD_ROOT}/bin/${ICU_TOOLCHAIN_NAME}${ANDROIDVER}-clang"
            "RANLIB=${NDK_STANDARD_ROOT}/bin/${ICU_TOOLCHAIN_NAME}-ranlib"
            "AR=${NDK_STANDARD_ROOT}/bin/${ICU_TOOLCHAIN_NAME}-ar"
            "CPPFLAGS=${ANDROID_COMPILER_FLAGS}"
            "LDFLAGS=${ANDROID_LINKER_FLAGS} -lc -lstdc++"
            )
        set(ICU_ADDITIONAL_OPTS --disable-tools --host=${ICU_TOOLCHAIN_NAME}${ANDROIDVER} --with-cross-build=${OPENMW_ICU_HOST_BUILD_DIR})
    endif()
    include(ExternalProject)
    ExternalProject_Add(icu
        URL https://github.com/unicode-org/icu/archive/refs/tags/release-70-1.zip
        URL_HASH MD5=49d5e2e5bab93ae1a4b56e916150544c
        SOURCE_DIR fetched/icu
        CONFIGURE_COMMAND ${CMAKE_COMMAND} -E env ${ICU_ENV}
            <SOURCE_DIR>/icu4c/source/configure --enable-static --disable-shared
            --disable-tests --disable-samples --disable-icuio --disable-extras ${ICU_ADDITIONAL_OPTS}
        BUILD_COMMAND make
        INSTALL_COMMAND ""
    )
    ExternalProject_Get_Property(icu SOURCE_DIR BINARY_DIR)
    set(ICU_INCLUDE_DIRS
        ${SOURCE_DIR}/icu4c/source/common
        ${SOURCE_DIR}/icu4c/source/i18n
        PARENT_SCOPE
    )
    foreach(ICULIB data uc i18n)
        add_library(ICU::${ICULIB} STATIC IMPORTED GLOBAL)
        set_target_properties(ICU::${ICULIB} PROPERTIES IMPORTED_LOCATION
            ${BINARY_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}icu${ICULIB}${CMAKE_STATIC_LIBRARY_SUFFIX})
        add_dependencies(ICU::${ICULIB} icu)
    endforeach()
    set(ICU_LIBRARIES ICU::i18n ICU::uc ICU::data PARENT_SCOPE)
endif()
