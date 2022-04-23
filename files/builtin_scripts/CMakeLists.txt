if (NOT DEFINED OPENMW_RESOURCES_ROOT)
    return()
endif()

set(LUA_BUILTIN_FILES
    builtin.omwscripts

    openmw_aux/util.lua
    openmw_aux/time.lua
    openmw_aux/calendar.lua

    scripts/omw/ai.lua
    scripts/omw/camera.lua
    scripts/omw/head_bobbing.lua
    scripts/omw/third_person.lua
    scripts/omw/console/player.lua
    scripts/omw/console/global.lua
    scripts/omw/console/local.lua

    l10n/Calendar/en.yaml
)

foreach (f ${LUA_BUILTIN_FILES})
    copy_resource_file("${CMAKE_CURRENT_SOURCE_DIR}/${f}" "${OPENMW_RESOURCES_ROOT}" "resources/vfs/${f}")
endforeach (f)

