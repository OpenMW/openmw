include_directories(
 include
 ${MYGUI_SOURCE_DIR}/MyGUIEngine/include
 ${OGRE_INCLUDE_DIR}
)
include(MyGUI.OgrePlatform.list)
add_library(MyGUIOgrePlatform ${HEADER_FILES} ${SOURCE_FILES})
target_link_libraries(MyGUIOgrePlatform ${OGRE_LIBRARIES})
link_directories(${OGRE_LIB_DIR})
