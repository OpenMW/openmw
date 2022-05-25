if (NOT DEFINED OPENMW_RESOURCES_ROOT)
    return()
endif()

set(BUILTIN_DATA_FILES
    textures/omw_menu_scroll_down.dds
    textures/omw_menu_scroll_up.dds
    textures/omw_menu_scroll_left.dds
    textures/omw_menu_scroll_right.dds
    textures/omw_menu_scroll_center_h.dds
    textures/omw_menu_scroll_center_v.dds

    l10n/Calendar/en.yaml
    l10n/Interface/en.yaml

    openmw_aux/util.lua
    openmw_aux/time.lua
    openmw_aux/calendar.lua
    openmw_aux/ui.lua

    builtin.omwscripts

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
    scripts/omw/mwui/constants.lua
    scripts/omw/mwui/borders.lua
    scripts/omw/mwui/filters.lua
    scripts/omw/mwui/text.lua
    scripts/omw/mwui/textEdit.lua
    scripts/omw/mwui/space.lua
    scripts/omw/mwui/init.lua

    shaders/main.omwfx
    shaders/displaydepth.omwfx
)

foreach (f ${BUILTIN_DATA_FILES})
    copy_resource_file("${CMAKE_CURRENT_SOURCE_DIR}/${f}" "${OPENMW_RESOURCES_ROOT}" "resources/vfs/${f}")
endforeach (f)
