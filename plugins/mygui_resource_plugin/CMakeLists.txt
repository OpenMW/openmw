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

if(WIN32)
  if(MSVC)
     # from top-level CMakelists.txt:
     # 4305 - Truncating value (double to float, for example)
    set_target_properties(${MYGUI_RESOURCE_PLUGIN_LIBRARY} PROPERTIES COMPILE_FLAGS "/wd4305")
  endif(MSVC)
endif(WIN32)

set_target_properties(${MYGUI_RESOURCE_PLUGIN_LIBRARY} PROPERTIES PREFIX "")

target_link_libraries(${MYGUI_RESOURCE_PLUGIN_LIBRARY}
    ${OGRE_LIBRARIES}
    ${MYGUI_LIBRARIES}
    components
)
