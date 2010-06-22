#include <MyGUI.h>

// The global GUI object
MyGUI::Gui *mGUI;

// This is used to determine if we are displaying any gui elements
// right now. If we are (and guiMode > 0), we redirect mouse/keyboard
// input into MyGUI.
int32_t guiMode = 0;

#include "../gui/cpp_mygui.cpp"
#include "../terrain/cpp_terrain.cpp"
