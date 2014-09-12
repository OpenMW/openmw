set (MYGUI_RESOURCE_PLUGIN_SOURCES
    plugin.hpp
    plugin.cpp
    plugin_export.cpp
)

set (MYGUI_RESOURCE_PLUGIN_LIBRARY
    Plugin_MyGUI_OpenMW_Resources
)

add_definitions("-D_USRDLL -DMYGUI_BUILD_DLL")

add_library(${MYGUI_RESOURCE_PLUGIN_LIBRARY}
    SHARED
    ${MYGUI_RESOURCE_PLUGIN_SOURCES}
    )

set_target_properties(${MYGUI_RESOURCE_PLUGIN_LIBRARY} PROPERTIES PREFIX "")

target_link_libraries(${MYGUI_RESOURCE_PLUGIN_LIBRARY}
    ${OGRE_LIBRARIES}
    ${MYGUI_LIBRARIES}
    components
)
