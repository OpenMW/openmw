# Minimal MyGUI build system for OpenMW

cmake_minimum_required(VERSION 2.6)
project(MYGUI)

# Use Freetype
find_package(Freetype REQUIRED)
set(MYGUI_USE_FREETYPE TRUE)

# Use Ogre
add_definitions("-DMYGUI_OGRE_PLATFORM")

# Static linking
set(MYGUI_LIB_TYPE STATIC)
add_definitions(-DMYGUI_STATIC)

# Setup MyGUIEngine project
add_subdirectory(MyGUIEngine)

# Setup OgrePlatform
add_subdirectory(OgrePlatform)

# Copy resource files into the build directory
set(SDIR ${CMAKE_CURRENT_SOURCE_DIR}/openmw_resources)
if (APPLE)
	set(DDIR ${APP_BUNDLE_DIR}/Contents/Resources/resources/mygui)
else (APPLE)
	set(DDIR ${OpenMW_BINARY_DIR}/resources/mygui)
endif (APPLE)

configure_file("${SDIR}/bigbars.png" "${DDIR}/bigbars.png" COPYONLY)
configure_file("${SDIR}/black.png" "${DDIR}/black.png" COPYONLY)
configure_file("${SDIR}/Comic.TTF" "${DDIR}/Comic.TTF" COPYONLY)
configure_file("${SDIR}/core.skin" "${DDIR}/core.skin" COPYONLY)
configure_file("${SDIR}/core.xml" "${DDIR}/core.xml" COPYONLY)
configure_file("${SDIR}/mwpointer.png" "${DDIR}/mwpointer.png" COPYONLY)
configure_file("${SDIR}/mwgui.png" "${DDIR}/mwgui.png" COPYONLY)
configure_file("${SDIR}/openmw_box.skin.xml" "${DDIR}/openmw_box.skin.xml" COPYONLY)
configure_file("${SDIR}/openmw_button.skin.xml" "${DDIR}/openmw_button.skin.xml" COPYONLY)
configure_file("${SDIR}/openmw_list.skin.xml" "${DDIR}/openmw_list.skin.xml" COPYONLY)
configure_file("${SDIR}/openmw_edit.skin.xml" "${DDIR}/openmw_edit.skin.xml" COPYONLY)
configure_file("${SDIR}/openmw_console_layout.xml" "${DDIR}/openmw_console_layout.xml" COPYONLY)
configure_file("${SDIR}/openmw_console.skin.xml" "${DDIR}/openmw_console.skin.xml" COPYONLY)
configure_file("${SDIR}/openmw.font.xml" "${DDIR}/openmw.font.xml" COPYONLY)
configure_file("${SDIR}/openmw_hud_box.skin.xml" "${DDIR}/openmw_hud_box.skin.xml" COPYONLY)
configure_file("${SDIR}/openmw_hud_energybar.skin.xml" "${DDIR}/openmw_hud_energybar.skin.xml" COPYONLY)
configure_file("${SDIR}/openmw_hud_layout.xml" "${DDIR}/openmw_hud_layout.xml" COPYONLY)
configure_file("${SDIR}/openmw_text_input_layout.xml" "${DDIR}/openmw_text_input_layout.xml" COPYONLY)
configure_file("${SDIR}/openmw_infobox_layout.xml" "${DDIR}/openmw_infobox_layout.xml" COPYONLY)
configure_file("${SDIR}/openmw_chargen_race_layout.xml" "${DDIR}/openmw_chargen_race_layout.xml" COPYONLY)
configure_file("${SDIR}/openmw_chargen_class_layout.xml" "${DDIR}/openmw_chargen_class_layout.xml" COPYONLY)
configure_file("${SDIR}/openmw_chargen_generate_class_result_layout.xml" "${DDIR}/openmw_chargen_generate_class_result_layout.xml" COPYONLY)
configure_file("${SDIR}/openmw_chargen_create_class_layout.xml" "${DDIR}/openmw_chargen_create_class_layout.xml" COPYONLY)
configure_file("${SDIR}/openmw_chargen_select_specialization_layout.xml" "${DDIR}/openmw_chargen_select_specialization_layout.xml" COPYONLY)
configure_file("${SDIR}/openmw_chargen_select_attribute_layout.xml" "${DDIR}/openmw_chargen_select_attribute_layout.xml" COPYONLY)
configure_file("${SDIR}/openmw_chargen_select_skill_layout.xml" "${DDIR}/openmw_chargen_select_skill_layout.xml" COPYONLY)
configure_file("${SDIR}/openmw_chargen_class_description_layout.xml" "${DDIR}/openmw_chargen_class_description_layout.xml" COPYONLY)
configure_file("${SDIR}/openmw_chargen_birth_layout.xml" "${DDIR}/openmw_chargen_birth_layout.xml" COPYONLY)
configure_file("${SDIR}/openmw_chargen_review_layout.xml" "${DDIR}/openmw_chargen_review_layout.xml" COPYONLY)
configure_file("${SDIR}/openmw_dialogue_window_layout.xml" "${DDIR}/openmw_dialogue_window_layout.xml" COPYONLY)
configure_file("${SDIR}/openmw_inventory_window_layout.xml" "${DDIR}/openmw_inventory_window_layout.xml" COPYONLY)
configure_file("${SDIR}/openmw_layers.xml" "${DDIR}/openmw_layers.xml" COPYONLY)
configure_file("${SDIR}/openmw_mainmenu_layout.xml" "${DDIR}/openmw_mainmenu_layout.xml" COPYONLY)
configure_file("${SDIR}/openmw_mainmenu_skin.xml" "${DDIR}/openmw_mainmenu_skin.xml" COPYONLY)
configure_file("${SDIR}/openmw_map_window_layout.xml" "${DDIR}/openmw_map_window_layout.xml" COPYONLY)
configure_file("${SDIR}/openmw.pointer.xml" "${DDIR}/openmw.pointer.xml" COPYONLY)
configure_file("${SDIR}/openmw_progress.skin.xml" "${DDIR}/openmw_progress.skin.xml" COPYONLY)
configure_file("${SDIR}/openmw_stats_window_layout.xml" "${DDIR}/openmw_stats_window_layout.xml" COPYONLY)
configure_file("${SDIR}/openmw_text.skin.xml" "${DDIR}/openmw_text.skin.xml" COPYONLY)
configure_file("${SDIR}/openmw_windows.skin.xml" "${DDIR}/openmw_windows.skin.xml" COPYONLY)
configure_file("${SDIR}/openmw_messagebox_layout.xml" "${DDIR}/openmw_messagebox_layout.xml" COPYONLY)
configure_file("${SDIR}/smallbars.png" "${DDIR}/smallbars.png" COPYONLY)
configure_file("${SDIR}/transparent.png" "${DDIR}/transparent.png" COPYONLY)
configure_file("${SDIR}/VeraMono.ttf" "${DDIR}/VeraMono.ttf" COPYONLY)
