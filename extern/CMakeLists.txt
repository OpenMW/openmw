# SPDX-License-Identifier: GPL-3.0-or-later

set_directory_properties(PROPERTIES EXCLUDE_FROM_ALL on)
set(CMAKE_CXX_CLANG_TIDY "")

if(NOT OPENMW_USE_SYSTEM_BULLET)

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
    FetchContent_MakeAvailable(bullet)

    set(BULLET_INCLUDE_DIRS ${bullet_SOURCE_DIR}/src PARENT_SCOPE)

    # The order here is important to work around a bug in Bullet:
    # https://github.com/bulletphysics/bullet3/issues/3233
    set(BULLET_LIBRARIES BulletCollision LinearMath PARENT_SCOPE)
endif()

if(NOT OPENMW_USE_SYSTEM_MYGUI)

    set(MYGUI_RENDERSYSTEM 4 CACHE STRING "")
    set(MYGUI_DISABLE_PLUGINS TRUE CACHE BOOL "")
    set(MYGUI_BUILD_DEMOS OFF CACHE BOOL "")
    set(MYGUI_BUILD_PLUGINS OFF CACHE BOOL "")
    set(MYGUI_BUILD_TOOLS OFF CACHE BOOL "")
    set(MYGUI_DONT_USE_OBSOLETE ON CACHE BOOL "")

    if(MYGUI_STATIC)
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    else()
        set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
    endif()

    include(FetchContent)
    FetchContent_Declare(mygui
        URL https://github.com/MyGUI/mygui/archive/refs/tags/MyGUI3.4.3.zip
        URL_HASH SHA512=c804ef665e786076582367f171082cd2181fdbd214300ddcca4a4245c5a0f45e62e72778ee2d96ec46b393e22477dd729f9bb3006e6eecbf536674e34a057721
        SOURCE_DIR fetched/mygui
    )
    FetchContent_MakeAvailable(mygui)

    set(MyGUI_INCLUDE_DIRS ${mygui_SOURCE_DIR}/MyGUIEngine/include PARENT_SCOPE)
    set(MyGUI_LIBRARIES MyGUIEngine PARENT_SCOPE)
endif()

if(NOT OPENMW_USE_SYSTEM_OSG)

    set(BUILD_OSG_APPLICATIONS OFF CACHE BOOL "")
    set(BUILD_OSG_DEPRECATED_SERIALIZERS OFF CACHE BOOL "")
    set(OSG_FIND_3RD_PARTY_DEPS OFF CACHE BOOL "")

    set(BUILD_OSG_PLUGINS_BY_DEFAULT OFF CACHE BOOL "")
    set(BUILD_OSG_PLUGIN_BMP ON CACHE BOOL "")
    set(BUILD_OSG_PLUGIN_DAE ON CACHE BOOL "")
    set(BUILD_OSG_PLUGIN_DDS ON CACHE BOOL "")
    set(BUILD_OSG_PLUGIN_FREETYPE ON CACHE BOOL "")
    set(BUILD_OSG_PLUGIN_JPEG ON CACHE BOOL "")
    set(BUILD_OSG_PLUGIN_KTX ON CACHE BOOL "")
    set(BUILD_OSG_PLUGIN_OSG ON CACHE BOOL "")
    set(BUILD_OSG_PLUGIN_PNG ON CACHE BOOL "")
    set(BUILD_OSG_PLUGIN_TGA ON CACHE BOOL "")

    set(OSG_USE_FLOAT_MATRIX OFF CACHE BOOL "")
    set(OSG_USE_FLOAT_PLANE OFF CACHE BOOL "")
    set(OSG_USE_FLOAT_QUAT OFF CACHE BOOL "")

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
    FetchContent_MakeAvailable(osg)

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

    include(FetchContent)
    FetchContent_Declare(recastnavigation
        URL https://github.com/OpenMW/recastnavigation/archive/4cdcda7867fa5c65cf3d5808f92f0441e134d849.zip
        URL_HASH SHA512=740fe8ae4205d88b96f6cfa50b553633e8d59ca5b5df0be90b0c3faf555d035f11b20633295ac49ba4dfae1fd07cb9b6353bb83aa61d2e0dfcb5827effc0e212
        SOURCE_DIR fetched/recastnavigation
    )
    FetchContent_MakeAvailable(recastnavigation)
endif()

if (NOT OPENMW_USE_SYSTEM_SQLITE3)
    include(FetchContent)
    FetchContent_Declare(sqlite3
        URL https://www.sqlite.org/2023/sqlite-amalgamation-3410100.zip
        URL_HASH SHA512=7de291709e88fffc693cf24ac675950cfc35c1bf7631cfea95167105720a05cf37fb943c57c5c985db2eeaa57b31894b3c0df98a7bd2939b5746fc5a24b5ae87
        SOURCE_DIR fetched/sqlite3
    )
    FetchContent_MakeAvailable(sqlite3)

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

    set(BENCHMARK_ENABLE_TESTING OFF)
    set(BENCHMARK_ENABLE_INSTALL OFF)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF)
    set(BENCHMARK_ENABLE_WERROR OFF)

    include(FetchContent)
    FetchContent_Declare(benchmark
        URL https://github.com/google/benchmark/archive/refs/tags/v1.8.3.zip
        URL_HASH SHA512=d73587ad9c49338749e1d117a6f8c7ff9c603a91a2ffa91a7355c7df7dea82710b9a810d34ddfef20973ecdc77092ec10fb2b4e4cc8d2e7810cbed79617b3828
        SOURCE_DIR fetched/benchmark
    )
    FetchContent_MakeAvailable(benchmark)
endif()

if (NOT OPENMW_USE_SYSTEM_YAML_CPP)
    if(YAML_CPP_STATIC)
        set(YAML_BUILD_SHARED_LIBS OFF)
    else()
        set(YAML_BUILD_SHARED_LIBS ON)
    endif()

    include(FetchContent)
    # One commit after the 0.8.0 tag for CMake 4.x compatibility
    FetchContent_Declare(yaml-cpp
        URL https://github.com/jbeder/yaml-cpp/archive/c2680200486572baf8221ba052ef50b58ecd816e.zip
        URL_HASH SHA512=9eb328e37ff547e7f681d64e3a8f268a8a5645cb1c258106d1d9f959eec498dfc7c466690c56f210ad7d5fb95b806a30c87ccbb019cccf5c3c6135cfc3c13fe1
        SOURCE_DIR fetched/yaml-cpp
    )
    FetchContent_MakeAvailable(yaml-cpp)

    if (MSVC AND PRECOMPILE_HEADERS_WITH_MSVC)
        target_precompile_headers(yaml-cpp PRIVATE <algorithm>)
    endif()
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

if ((BUILD_COMPONENTS_TESTS OR BUILD_OPENCS_TESTS OR BUILD_OPENMW_TESTS) AND NOT OPENMW_USE_SYSTEM_GOOGLETEST)

    include(FetchContent)
    FetchContent_Declare(googletest
        URL https://github.com/google/googletest/archive/refs/tags/release-1.12.1.zip
        URL_HASH SHA512=1479ea2f3172c622c0ca305f5b2bc45a42941221ec0ac7865e6d6d020ec4d008d952fc64e01a4c5138d7bed4148cf75596f25bb9e9044a98bbbf5662053ea11c
        SOURCE_DIR fetched/googletest
    )
    if (MSVC)
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    endif()
    FetchContent_MakeAvailable(googletest)

    add_library(GTest::GTest ALIAS gtest)
    add_library(GMock::GMock ALIAS gmock)
endif()
