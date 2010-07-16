add_executable(mygui_test
  main.cpp
  ${BSA}  ${BSA_HEADER}
  ${OENGINE_OGRE}
  ${OENGINE_GUI}
  ${MANGLE_INPUT}
)
target_link_libraries(mygui_test
  ${OGRE_LIBRARIES}
  ${OIS_LIBRARIES}
  MyGUIEngine
  MyGUI.OgrePlatform
)
