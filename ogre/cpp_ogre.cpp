/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (cpp_ogre.cpp) is part of the OpenMW package.

  OpenMW is distributed as free software: you can redistribute it
  and/or modify it under the terms of the GNU General Public License
  version 3, as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  version 3 along with this program. If not, see
  http://www.gnu.org/licenses/ .

 */

#include <Ogre.h>
#include <iostream>

#include <Ogre.h>
#include <OgreConfigFile.h>
#include <OgreStringConverter.h>
#include <OgreException.h>
#include <OgreArchive.h>
#include <OgreArchiveFactory.h>

#include <OIS/OIS.h>

#include <MyGUI.h>

#include "dbg.h"

using namespace Ogre;

RenderWindow* mWindow;
Root *mRoot;
SceneManager *mSceneMgr;
Camera *mCamera;
Viewport *vp;

ColourValue g_ambient;
int g_lightOn = 0;

/*
int g_spotOn = 0;
Light *g_light;
*/

// Set to nonzero if debug mode is enabled
int g_isDebug = 0;

OIS::InputManager *mInputManager;
OIS::Mouse *mMouse;
OIS::Keyboard *mKeyboard;

// The global GUI object
MyGUI::Gui *mGUI;

// This is used to determine if we are displaying any gui elements
// right now. If we are (and guiMode > 0), we redirect mouse/keyboard
// input into MyGUI.
int32_t guiMode = 0;

// Root node for all objects added to the scene. This is rotated so
// that the OGRE coordinate system matches that used internally in
// Morrowind.
SceneNode *root;

// Include the other parts of the code, and make one big happy object
// file. This is extremely against the grain of C++ "recomended
// practice", but I don't care.
#include "../gui/cpp_mygui.cpp"
#include "cpp_framelistener.cpp"
#include "cpp_bsaarchive.cpp"
#include "cpp_interface.cpp"
#include "../terrain/cpp_terrain.cpp"
