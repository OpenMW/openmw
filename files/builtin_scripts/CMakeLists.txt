if (NOT DEFINED OPENMW_RESOURCES_ROOT)
    return()
endif()

# Copy resource files into the build directory
set(SDIR ${CMAKE_CURRENT_SOURCE_DIR})
set(DDIRRELATIVE resources/vfs)
copy_all_resource_files(${CMAKE_CURRENT_SOURCE_DIR} ${OPENMW_RESOURCES_ROOT} ${DDIRRELATIVE} "builtin.omwscripts")

set(DDIRRELATIVE resources/vfs/openmw_aux)
copy_all_resource_files(${CMAKE_CURRENT_SOURCE_DIR} ${OPENMW_RESOURCES_ROOT} ${DDIRRELATIVE} "openmw_aux/util.lua")

set(LUA_SCRIPTS_FILES
    scripts/omw/camera.lua
    scripts/omw/head_bobbing.lua
    scripts/omw/third_person.lua
)

set(DDIRRELATIVE resources/vfs/scripts/omw)
copy_all_resource_files(${CMAKE_CURRENT_SOURCE_DIR} ${OPENMW_RESOURCES_ROOT} ${DDIRRELATIVE} "${LUA_SCRIPTS_FILES}")
