
# Copy resource files into the build directory
set(SDIR ${CMAKE_CURRENT_SOURCE_DIR})
set(DDIR ${OpenMW_BINARY_DIR}/resources/mygui)

set(MYGUI_FILES
    bigbars.png
    black.png
    core.skin
    core.xml
    EBGaramond-Regular.ttf
    mwgui.png
    Obliviontt.zip
    openmw_alchemy_window.layout
    openmw_book.layout
    openmw_box.skin.xml
    openmw_button.skin.xml
    openmw_chargen_birth.layout
    openmw_chargen_class_description.layout
    openmw_chargen_class.layout
    openmw_chargen_create_class.layout
    openmw_chargen_generate_class_result.layout
    openmw_chargen_race.layout
    openmw_chargen_review.layout
    openmw_chargen_select_attribute.layout
    openmw_chargen_select_skill.layout
    openmw_chargen_select_specialization.layout
    openmw_confirmation_dialog.layout
    openmw_console.layout
    openmw_console.skin.xml
    openmw_container_window.layout
    openmw_count_window.layout
    openmw_dialogue_window.layout
    openmw_dialogue_window_skin.xml
    openmw_edit.skin.xml
    openmw_font.xml
    openmw_hud_box.skin.xml
    openmw_hud_energybar.skin.xml
    openmw_hud.layout
    openmw_infobox.layout
    openmw_interactive_messagebox.layout
    openmw_inventory_window.layout
    openmw_journal.layout
    openmw_journal_skin.xml
    openmw_layers.xml
    openmw_list.skin.xml
    openmw_mainmenu.layout
    openmw_mainmenu_skin.xml
    openmw_map_window.layout
    openmw_map_window_skin.xml
    openmw_messagebox.layout
    openmw_pointer.xml
    openmw_progress.skin.xml
    openmw_resources.xml
    openmw_scroll.layout
    openmw_scroll_skin.xml
    openmw_settings_window.layout
    openmw_settings.xml
    openmw_spell_window.layout
    openmw_stats_window.layout
    openmw_text_input.layout
    openmw_text.skin.xml
    openmw_tooltips.layout
    openmw_trade_window.layout
    openmw_spell_buying_window.layout
    openmw_windows.skin.xml
    openmw_quickkeys_menu.layout
    openmw_quickkeys_menu_assign.layout
    openmw_itemselection_dialog.layout
    openmw_magicselection_dialog.layout
    openmw_spell_buying_window.layout
    openmw_loading_screen.layout
    openmw_levelup_dialog.layout
    openmw_wait_dialog.layout
    openmw_wait_dialog_progressbar.layout
    openmw_spellcreation_dialog.layout
    openmw_edit_effect.layout
    openmw_enchanting_dialog.layout
    openmw_trainingwindow.layout
    openmw_travel_window.layout
    openmw_persuasion_dialog.layout
    smallbars.png
    VeraMono.ttf
    markers.png
)


copy_all_files(${CMAKE_CURRENT_SOURCE_DIR} ${DDIR} "${MYGUI_FILES}")
