if (NOT DEFINED OPENMW_RESOURCES_ROOT)
    return()
endif()

# Copy resource files into the build directory
set(SDIR ${CMAKE_CURRENT_SOURCE_DIR})
set(DDIRRELATIVE resources/vfs)
copy_all_resource_files(${CMAKE_CURRENT_SOURCE_DIR} ${OPENMW_RESOURCES_ROOT} ${DDIRRELATIVE} "builtin.omwscripts")

set(LUA_AUX_FILES
    openmw_aux/util.lua
    openmw_aux/time.lua
    openmw_aux/calendar.lua
)

set(DDIRRELATIVE resources/vfs/openmw_aux)
copy_all_resource_files(${CMAKE_CURRENT_SOURCE_DIR} ${OPENMW_RESOURCES_ROOT} ${DDIRRELATIVE} "${LUA_AUX_FILES}")

set(LUA_SCRIPTS_FILES
    scripts/omw/ai.lua
    scripts/omw/camera.lua
    scripts/omw/head_bobbing.lua
    scripts/omw/third_person.lua
)

set(DDIRRELATIVE resources/vfs/scripts/omw)
copy_all_resource_files(${CMAKE_CURRENT_SOURCE_DIR} ${OPENMW_RESOURCES_ROOT} ${DDIRRELATIVE} "${LUA_SCRIPTS_FILES}")

set(DDIRRELATIVE resources/vfs/i18n/Calendar)
copy_all_resource_files(${CMAKE_CURRENT_SOURCE_DIR} ${OPENMW_RESOURCES_ROOT} ${DDIRRELATIVE} "i18n/Calendar/en.lua")
