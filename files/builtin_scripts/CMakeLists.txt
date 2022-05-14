if (NOT DEFINED OPENMW_RESOURCES_ROOT)
    return()
endif()

set(LUA_BUILTIN_FILES
    builtin.omwscripts

    openmw_aux/util.lua
    openmw_aux/time.lua
    openmw_aux/calendar.lua
    openmw_aux/ui.lua

    scripts/omw/ai.lua
    scripts/omw/camera.lua
    scripts/omw/head_bobbing.lua
    scripts/omw/third_person.lua
    scripts/omw/console/player.lua
    scripts/omw/console/global.lua
    scripts/omw/console/local.lua
    scripts/omw/settings/player.lua
    scripts/omw/settings/global.lua
    scripts/omw/settings/common.lua
    scripts/omw/settings/render.lua
    scripts/omw/settings/renderers.lua

    l10n/Calendar/en.yaml

    scripts/omw/mwui/constants.lua
    scripts/omw/mwui/borders.lua
    scripts/omw/mwui/box.lua
    scripts/omw/mwui/text.lua
    scripts/omw/mwui/textEdit.lua
    scripts/omw/mwui/space.lua
    scripts/omw/mwui/init.lua
)

foreach (f ${LUA_BUILTIN_FILES})
    copy_resource_file("${CMAKE_CURRENT_SOURCE_DIR}/${f}" "${OPENMW_RESOURCES_ROOT}" "resources/vfs/${f}")
endforeach (f)
