if (NOT DEFINED OPENMW_RESOURCES_ROOT)
    return()
endif()

set(BUILTIN_DATA_MW_FILES
    # Month names and date formatting
    l10n/Calendar/de.yaml
    l10n/Calendar/en.yaml
    l10n/Calendar/gmst.yaml
    l10n/Calendar/ru.yaml
    l10n/Calendar/sv.yaml
    l10n/Calendar/fr.yaml
    l10n/Calendar/pl.yaml

    # Generic UI messages that can be reused by mods
    l10n/Interface/gmst.yaml

    # L10n for OpenMW menus and non-game-specific messages
    l10n/OMWEngine/gmst.yaml

    # Game-specific settings for calendar.lua
    openmw_aux/calendarconfig.lua

    builtin.omwscripts

    scripts/omw/cellhandlers.lua
    scripts/omw/combat/common.lua
    scripts/omw/combat/global.lua
    scripts/omw/combat/local.lua
    scripts/omw/combat/menu.lua
    scripts/omw/music/helpers.lua
    scripts/omw/music/music.lua
    scripts/omw/music/settings.lua
    scripts/omw/playerskillhandlers.lua
)

foreach (f ${BUILTIN_DATA_MW_FILES})
    copy_resource_file("${CMAKE_CURRENT_SOURCE_DIR}/${f}" "${OPENMW_RESOURCES_ROOT}" "resources/vfs-mw/${f}")
endforeach (f)
