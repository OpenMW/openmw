add_executable(mygui_test
  main.cpp
  ${BSA}  ${BSA_HEADER}
  ${OGRE} ${OGRE_HEADER}
  ${EGUI} ${EGUI_HEADER}
)
target_link_libraries(mygui_test
  ${OGRE_LIBRARIES}
  ${OIS_LIBRARIES}
  MyGUIEngine
  MyGUI.OgrePlatform
)
