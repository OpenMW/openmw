# SPDX-License-Identifier: GPL-3.0-or-later

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
        URL_HASH MD5=7711bce9a49c289a08ecda34eaa0f32e
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
        URL_HASH MD5=0a64c9cccc8f96dc8c08172175e68e1c
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

    # branch OpenSceneGraph-3.6 on 23 Jan 2021.
    include(FetchContent)
    FetchContent_Declare(osg
        URL https://github.com/OpenMW/osg/archive/e65f47c4ab3a0b53cc19f517961671e5f840a08d.zip
        URL_HASH MD5=0c967fe48d80744f6956f6b0b67ef7c6
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
        URL_HASH MD5=af905d121ef9d1cdfa979b0495cba059
        SOURCE_DIR fetched/recastnavigation
    )
    FetchContent_MakeAvailableExcludeFromAll(recastnavigation)
endif()
