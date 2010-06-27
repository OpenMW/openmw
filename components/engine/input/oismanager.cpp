#include "oismanager.hpp"
#include <assert.h>
#include <string>
#include <sstream>
#include <iostream>

#include "OgreRenderWindow.h"

#ifdef __APPLE_CC__
#include <Carbon/Carbon.h>
#endif

using namespace Input;
using namespace Ogre;
using namespace OIS;

using namespace std;

OISManager::OISManager(Render::OgreRenderer &rend)
{
  RenderWindow *window = rend.getWindow();
  assert(window);

  size_t windowHnd;
  window->getCustomAttribute("WINDOW", &windowHnd);

  std::ostringstream windowHndStr;
  ParamList pl;

  windowHndStr << windowHnd;
  pl.insert(std::make_pair(std::string("WINDOW"), windowHndStr.str()));

  // Non-exclusive mouse and keyboard input in debug mode. Debug mode
  // isn't implemented yet though.
  if(false)
    {
#if defined OIS_WIN32_PLATFORM
      pl.insert(std::make_pair(std::string("w32_mouse"),
                               std::string("DISCL_FOREGROUND" )));
      pl.insert(std::make_pair(std::string("w32_mouse"),
                               std::string("DISCL_NONEXCLUSIVE")));
      pl.insert(std::make_pair(std::string("w32_keyboard"),
                               std::string("DISCL_FOREGROUND")));
      pl.insert(std::make_pair(std::string("w32_keyboard"),
                               std::string("DISCL_NONEXCLUSIVE")));
#elif defined OIS_LINUX_PLATFORM
      pl.insert(std::make_pair(std::string("x11_mouse_grab"),
                               std::string("false")));
      pl.insert(std::make_pair(std::string("x11_mouse_hide"),
                               std::string("false")));
      pl.insert(std::make_pair(std::string("x11_keyboard_grab"),
                               std::string("false")));
      pl.insert(std::make_pair(std::string("XAutoRepeatOn"),
                               std::string("true")));
#endif
    }

#ifdef __APPLE_CC__
  // Give the application window focus to receive input events
  ProcessSerialNumber psn = { 0, kCurrentProcess };
  TransformProcessType(&psn, kProcessTransformToForegroundApplication);
  SetFrontProcess(&psn);
#endif

  inputMgr = InputManager::createInputSystem( pl );

  // Create all devices
  keyboard = static_cast<Keyboard*>(inputMgr->createInputObject
                                    ( OISKeyboard, true ));
  mouse = static_cast<Mouse*>(inputMgr->createInputObject
                              ( OISMouse, true ));

  // Set mouse region
  const MouseState &ms = mouse->getMouseState();
  ms.width  = window->getWidth();
  ms.height = window->getHeight();
}

OISManager::~OISManager()
{
  if(inputMgr == NULL) return;

  // Kill the input systems. This will reset input options such as key
  // repetition.
  inputMgr->destroyInputObject(keyboard);
  inputMgr->destroyInputObject(mouse);
  InputManager::destroyInputSystem(inputMgr);
  inputMgr = NULL;
}
