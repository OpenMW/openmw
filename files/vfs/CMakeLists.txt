if (NOT DEFINED OPENMW_MYGUI_FILES_ROOT)
    return()
endif()

# Copy resource files into the build directory
set(SDIR ${CMAKE_CURRENT_SOURCE_DIR})
set(DDIRRELATIVE resources/vfs/textures)

set(TEXTURE_FILES
    textures/omw_menu_scroll_down.dds
    textures/omw_menu_scroll_up.dds
    textures/omw_menu_scroll_left.dds
    textures/omw_menu_scroll_right.dds
    textures/omw_menu_scroll_center_h.dds
    textures/omw_menu_scroll_center_v.dds
)

copy_all_resource_files(${CMAKE_CURRENT_SOURCE_DIR} ${OPENMW_MYGUI_FILES_ROOT} ${DDIRRELATIVE} "${TEXTURE_FILES}")
