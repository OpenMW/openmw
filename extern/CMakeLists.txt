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
    set(BUILD_BULLET3 OFF CACHE BOOL "")
    set(BUILD_EXTRAS OFF CACHE BOOL "")
    set(BUILD_OPENGL3_DEMOS OFF CACHE BOOL "")
    set(BUILD_UNIT_TESTS OFF CACHE BOOL "")
    set(BUILD_BULLET2_DEMOS OFF CACHE BOOL "")
    set(BUILD_CLSOCKET OFF CACHE BOOL "")
    set(BUILD_ENET OFF CACHE BOOL "")
    set(BUILD_CPU_DEMOS OFF CACHE BOOL "")
    set(BUILD_EGL OFF CACHE BOOL "")

    set(USE_DOUBLE_PRECISION ${BULLET_USE_DOUBLES} CACHE BOOL "")
    set(BULLET2_MULTITHREADING ON CACHE BOOL "")

    # Version 3.08 with the following changes:
    # 1. Fixes the linking of Threads:
    #    https://github.com/bulletphysics/bullet3/pull/3237
    # 2. Removes ~300 MiB of files not used here:
    #    rm -rf build3 data docs examples test Doxyfile
    include(FetchContent)
    FetchContent_Declare(bullet
        URL https://github.com/glebm/bullet3/archive/ed5256454f4f84bd2c1728c88ddb0405d614e7d2.zip
        URL_HASH MD5=e3c94fac35a7be885ad8843f828a0f96
        SOURCE_DIR fetched/bullet
    )
    FetchContent_MakeAvailableExcludeFromAll(bullet)

    set(BULLET_INCLUDE_DIRS ${bullet_SOURCE_DIR}/src PARENT_SCOPE)

    # The order here is important to work around a bug in Bullet:
    # https://github.com/bulletphysics/bullet3/issues/3233
    set(BULLET_LIBRARIES BulletCollision LinearMath PARENT_SCOPE)
endif()

if(NOT OPENMW_USE_SYSTEM_MYGUI)
    set(MYGUI_STATIC ON CACHE BOOL "")

    set(MYGUI_RENDERSYSTEM 4 CACHE STRING "")
    set(MYGUI_DISABLE_PLUGINS TRUE CACHE BOOL "")
    set(MYGUI_BUILD_DEMOS OFF CACHE BOOL "")
    set(MYGUI_BUILD_PLUGINS OFF CACHE BOOL "")
    set(MYGUI_BUILD_TOOLS OFF CACHE BOOL "")

    include(FetchContent)
    FetchContent_Declare(mygui
        URL https://github.com/MyGUI/mygui/archive/MyGUI3.4.0.zip
        URL_HASH MD5=9e990a4240430cbf567bfe73488a274e
        SOURCE_DIR fetched/mygui
    )
    FetchContent_MakeAvailableExcludeFromAll(mygui)

    set(MyGUI_INCLUDE_DIRS ${mygui_SOURCE_DIR}/MyGUIEngine/include PARENT_SCOPE)
    set(MyGUI_LIBRARIES MyGUIEngine PARENT_SCOPE)
endif()

if(NOT OPENMW_USE_SYSTEM_OSG)
    set(OSG_STATIC ON CACHE BOOL "")

    set(DYNAMIC_OPENTHREADS OFF CACHE BOOL "")
    set(DYNAMIC_OPENSCENEGRAPH OFF CACHE BOOL "")
    set(BUILD_OSG_APPLICATIONS OFF CACHE BOOL "")
    set(BUILD_OSG_DEPRECATED_SERIALIZERS OFF CACHE BOOL "")

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

    # branch OpenSceneGraph-3.6 on 18 Jan 2021.
    # + https://github.com/openscenegraph/OpenSceneGraph/pull/1032
    # + https://github.com/openscenegraph/OpenSceneGraph/pull/1033
    include(FetchContent)
    FetchContent_Declare(osg
        URL https://github.com/glebm/OpenSceneGraph/archive/041090d84d9d4b72f1202457fceae0ec6f79b663.zip
        URL_HASH MD5=2cbf8126b27a45a2a44efe9fa39090f3
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
