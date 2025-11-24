if (NOT DEFINED OPENMW_RESOURCES_ROOT)
    return()
endif()

set(BUILTIN_DATA_FILES
    textures/omw_map_color_palette.dds
    textures/omw_menu_scroll_down.dds
    textures/omw_menu_scroll_up.dds
    textures/omw_menu_scroll_left.dds
    textures/omw_menu_scroll_right.dds
    textures/omw_menu_scroll_center_h.dds
    textures/omw_menu_scroll_center_v.dds
    textures/omw_menu_icon_active.dds
    textures/omw_psx_button_circle.dds
    textures/omw_psx_button_dpad.dds
    textures/omw_psx_button_square.dds
    textures/omw_psx_button_triangle.dds
    textures/omw_psx_button_x.dds
    textures/omw_steam_button_a.dds
    textures/omw_steam_button_b.dds
    textures/omw_steam_button_dpad.dds
    textures/omw_steam_button_l1.dds
    textures/omw_steam_button_l2.dds
    textures/omw_steam_button_l3.dds
    textures/omw_steam_button_lstick.dds
    textures/omw_steam_button_menu.dds
    textures/omw_steam_button_r1.dds
    textures/omw_steam_button_r2.dds
    textures/omw_steam_button_r3.dds
    textures/omw_steam_button_rstick.dds
    textures/omw_steam_button_view.dds
    textures/omw_steam_button_x.dds
    textures/omw_steam_button_y.dds
    textures/omw_switch_button_l.dds
    textures/omw_switch_button_r.dds
    textures/omw_switch_button_zl.dds
    textures/omw_switch_button_zr.dds
    textures/omw_xbox_button_lb.dds
    textures/omw_xbox_button_lt.dds
    textures/omw_xbox_button_rb.dds
    textures/omw_xbox_button_rt.dds
    textures/omw/water_nm.png

    fonts/DejaVuFontLicense.txt
    fonts/DejaVuLGCSansMono.ttf
    fonts/DejaVuLGCSansMono.omwfont
    fonts/DemonicLetters.ttf
    fonts/DemonicLetters.omwfont
    fonts/DemonicLettersFontLicense.txt
    fonts/MysticCards.ttf
    fonts/MysticCards.omwfont
    fonts/MysticCardsFontLicense.txt

    # Default animation blending config
    animations/animation-config.yaml

    # Month names and date formatting
    l10n/Calendar/de.yaml
    l10n/Calendar/en.yaml
    l10n/Calendar/ru.yaml
    l10n/Calendar/sv.yaml
    l10n/Calendar/pl.yaml

    # Generic UI messages that can be reused by mods
    l10n/Interface/de.yaml
    l10n/Interface/en.yaml
    l10n/Interface/ru.yaml
    l10n/Interface/sv.yaml
    l10n/Interface/fr.yaml
    l10n/Interface/pl.yaml

    # L10n for scripts/omw
    l10n/OMWCamera/de.yaml
    l10n/OMWCamera/en.yaml
    l10n/OMWCamera/ru.yaml
    l10n/OMWCamera/sv.yaml
    l10n/OMWCamera/fr.yaml
    l10n/OMWCamera/pl.yaml

    l10n/OMWCombat/de.yaml
    l10n/OMWCombat/en.yaml
    l10n/OMWCombat/fr.yaml
    l10n/OMWCombat/ru.yaml
    l10n/OMWCombat/sv.yaml
    l10n/OMWCombat/pl.yaml

    l10n/OMWControls/de.yaml
    l10n/OMWControls/en.yaml
    l10n/OMWControls/fr.yaml
    l10n/OMWControls/ru.yaml
    l10n/OMWControls/sv.yaml
    l10n/OMWControls/pl.yaml

    # L10n for OpenMW menus and non-game-specific messages
    l10n/OMWEngine/de.yaml
    l10n/OMWEngine/en.yaml
    l10n/OMWEngine/ru.yaml
    l10n/OMWEngine/sv.yaml
    l10n/OMWEngine/fr.yaml
    l10n/OMWEngine/pl.yaml

    # L10n for music system
    l10n/OMWMusic/de.yaml
    l10n/OMWMusic/en.yaml
    l10n/OMWMusic/ru.yaml
    l10n/OMWMusic/sv.yaml
    l10n/OMWMusic/fr.yaml
    l10n/OMWMusic/pl.yaml

    # L10n for post-processing HUD and built-in shaders
    l10n/OMWShaders/de.yaml
    l10n/OMWShaders/en.yaml
    l10n/OMWShaders/ru.yaml
    l10n/OMWShaders/sv.yaml
    l10n/OMWShaders/fr.yaml
    l10n/OMWShaders/pl.yaml

    openmw_aux/util.lua
    openmw_aux/time.lua
    openmw_aux/calendar.lua
    openmw_aux/calendarconfig.lua
    openmw_aux/ui.lua

    builtin.omwscripts

    scripts/omw/activationhandlers.lua
    scripts/omw/ai.lua
    scripts/omw/camera/camera.lua
    scripts/omw/camera/head_bobbing.lua
    scripts/omw/camera/third_person.lua
    scripts/omw/camera/settings.lua
    scripts/omw/camera/move360.lua
    scripts/omw/camera/first_person_auto_switch.lua
    scripts/omw/console/global.lua
    scripts/omw/console/local.lua
    scripts/omw/console/player.lua
    scripts/omw/console/menu.lua
    scripts/omw/combat/interface.lua
    scripts/omw/mechanics/actorcontroller.lua
    scripts/omw/mechanics/animationcontroller.lua
    scripts/omw/mechanics/globalcontroller.lua
    scripts/omw/mechanics/playercontroller.lua
    scripts/omw/settings/menu.lua
    scripts/omw/music/actor.lua
    scripts/omw/settings/player.lua
    scripts/omw/settings/global.lua
    scripts/omw/settings/common.lua
    scripts/omw/settings/renderers.lua
    scripts/omw/mwui/constants.lua
    scripts/omw/mwui/borders.lua
    scripts/omw/mwui/filters.lua
    scripts/omw/mwui/text.lua
    scripts/omw/mwui/textEdit.lua
    scripts/omw/mwui/space.lua
    scripts/omw/mwui/init.lua
    scripts/omw/skillhandlers.lua
    scripts/omw/crimes.lua
    scripts/omw/ui.lua
    scripts/omw/usehandlers.lua
    scripts/omw/worldeventhandlers.lua
    scripts/omw/input/settings.lua
    scripts/omw/input/playercontrols.lua
    scripts/omw/input/actionbindings.lua
    scripts/omw/input/smoothmovement.lua
    scripts/omw/input/gamepadcontrols.lua
    
    shaders/adjustments.omwfx
    shaders/bloomlinear.omwfx
    shaders/debug.omwfx
    shaders/internal_distortion.omwfx

    mygui/core.skin
    mygui/core.xml
    mygui/core_layouteditor.xml
    mygui/openmw_alchemy_window.layout
    mygui/openmw_book.layout
    mygui/openmw_box.skin.xml
    mygui/openmw_button.skin.xml
    mygui/openmw_chargen_birth.layout
    mygui/openmw_chargen_class_description.layout
    mygui/openmw_chargen_class.layout
    mygui/openmw_chargen_create_class.layout
    mygui/openmw_chargen_generate_class_result.layout
    mygui/openmw_chargen_race.layout
    mygui/openmw_chargen_review.layout
    mygui/openmw_chargen_select_attribute.layout
    mygui/openmw_chargen_select_skill.layout
    mygui/openmw_chargen_select_specialization.layout
    mygui/openmw_confirmation_dialog.layout
    mygui/openmw_console.layout
    mygui/openmw_console.skin.xml
    mygui/openmw_container_window.layout
    mygui/openmw_controllerbuttons.layout
    mygui/openmw_count_window.layout
    mygui/openmw_dialogue_window.layout
    mygui/openmw_dialogue_window.skin.xml
    mygui/openmw_edit.skin.xml
    mygui/openmw_hud_box.skin.xml
    mygui/openmw_hud_energybar.skin.xml
    mygui/openmw_hud.layout
    mygui/openmw_infobox.layout
    mygui/openmw_interactive_messagebox.layout
    mygui/openmw_interactive_messagebox_notransp.layout
    mygui/openmw_inventory_tabs.layout
    mygui/openmw_inventory_window.layout
    mygui/openmw_journal.layout
    mygui/openmw_journal.skin.xml
    mygui/openmw_layers.xml
    mygui/openmw_lua.xml
    mygui/openmw_list.skin.xml
    mygui/openmw_mainmenu.layout
    mygui/openmw_mainmenu.skin.xml
    mygui/openmw_map_window.layout
    mygui/openmw_map_window.skin.xml
    mygui/openmw_messagebox.layout
    mygui/openmw_pointer.xml
    mygui/openmw_progress.skin.xml
    mygui/openmw_resources.xml
    mygui/openmw_scroll.layout
    mygui/openmw_scroll.skin.xml
    mygui/openmw_settings_window.layout
    mygui/openmw_settings.xml
    mygui/openmw_spell_window.layout
    mygui/openmw_stats_window.layout
    mygui/openmw_text_input.layout
    mygui/openmw_text.skin.xml
    mygui/openmw_tooltips.layout
    mygui/openmw_trade_window.layout
    mygui/openmw_spell_buying_window.layout
    mygui/openmw_windows.skin.xml
    mygui/openmw_quickkeys_menu.layout
    mygui/openmw_quickkeys_menu_assign.layout
    mygui/openmw_itemselection_dialog.layout
    mygui/openmw_magicselection_dialog.layout
    mygui/openmw_spell_buying_window.layout
    mygui/openmw_loading_screen.layout
    mygui/openmw_levelup_dialog.layout
    mygui/openmw_wait_dialog.layout
    mygui/openmw_wait_dialog_progressbar.layout
    mygui/openmw_spellcreation_dialog.layout
    mygui/openmw_edit_effect.layout
    mygui/openmw_enchanting_dialog.layout
    mygui/openmw_trainingwindow.layout
    mygui/openmw_travel_window.layout
    mygui/openmw_persuasion_dialog.layout
    mygui/openmw_merchantrepair.layout
    mygui/openmw_repair.layout
    mygui/openmw_companion_window.layout
    mygui/openmw_savegame_dialog.layout
    mygui/openmw_recharge_dialog.layout
    mygui/openmw_screen_fader.layout
    mygui/openmw_screen_fader_hit.layout
    mygui/openmw_edit_note.layout
    mygui/openmw_debug_window.layout
    mygui/openmw_debug_window.skin.xml
    mygui/openmw_postprocessor_hud.layout
    mygui/openmw_postprocessor_hud.skin.xml
    mygui/openmw_jail_screen.layout
    mygui/OpenMWResourcePlugin.xml
    mygui/skins.xml
)

foreach (f ${BUILTIN_DATA_FILES})
    copy_resource_file("${CMAKE_CURRENT_SOURCE_DIR}/${f}" "${OPENMW_RESOURCES_ROOT}" "resources/vfs/${f}")
endforeach (f)
