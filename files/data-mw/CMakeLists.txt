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

# Concat data/builtin.omwscripts and data-mw/builtin-extra.omwscripts to create vfs-mw/builtin.omwscripts
set(builtinBase "${CMAKE_CURRENT_SOURCE_DIR}/../data/builtin.omwscripts")
set(builtinExtra "${CMAKE_CURRENT_SOURCE_DIR}/builtin-extra.omwscripts")
set(builtinTemp "${CMAKE_BINARY_DIR}/tmp")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${builtinBase}" "${builtinExtra}")
file(COPY "${builtinBase}" DESTINATION "${builtinTemp}")
file(READ "${builtinExtra}" builtinContents)
file(APPEND "${builtinTemp}/builtin.omwscripts" "${builtinContents}")
copy_resource_file("${builtinTemp}/builtin.omwscripts" "${OPENMW_RESOURCES_ROOT}" "resources/vfs-mw/builtin.omwscripts")
