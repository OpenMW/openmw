#include "ois_driver.hpp"

#include <cassert>
#include <sstream>
#include <OgreRenderWindow.h>
#include <OIS/OIS.h>

#ifdef __APPLE_CC__
#include <Carbon/Carbon.h>
#endif

using namespace Mangle::Input;
using namespace OIS;

struct Mangle::Input::OISListener : OIS::KeyListener, OIS::MouseListener
{
  OISDriver &drv;

  OISListener(OISDriver &driver)
    : drv(driver) {}

  bool keyPressed( const OIS::KeyEvent &arg )
  {
    drv.makeEvent(Event::EV_KeyDown, arg.key, &arg);
    return true;
  }

  bool keyReleased( const OIS::KeyEvent &arg )
  {
    drv.makeEvent(Event::EV_KeyUp, arg.key, &arg);
    return true;
  }

  bool mousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
  {
    // Mouse button events are handled as key events
    // TODO: Translate mouse buttons into pseudo-keysyms
    drv.makeEvent(Event::EV_MouseDown, id, &arg);
    return true;
  }

  bool mouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
  {
    // TODO: ditto
    drv.makeEvent(Event::EV_MouseUp, id, &arg);
    return true;
  }

  bool mouseMoved( const OIS::MouseEvent &arg )
  {
    drv.makeEvent(Event::EV_MouseMove, -1, &arg);
    return true;
  }
};

OISDriver::OISDriver(Ogre::RenderWindow *window, bool exclusive)
{
  assert(window);

  size_t windowHnd;

  window->getCustomAttribute("WINDOW", &windowHnd);

  std::ostringstream windowHndStr;
  ParamList pl;

  windowHndStr << windowHnd;
  pl.insert(std::make_pair(std::string("WINDOW"), windowHndStr.str()));

  // Set non-exclusive mouse and keyboard input if the user requested
  // it.
  if(!exclusive)
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

  // Set up the input listener
  listener = new OISListener(*this);
  keyboard-> setEventCallback(listener);
  mouse-> setEventCallback(listener);
}

OISDriver::~OISDriver()
{
  // Delete the listener object
  delete listener;

  if(inputMgr == NULL) return;

  // Kill the input systems. This will reset input options such as key
  // repeat rate.
  inputMgr->destroyInputObject(keyboard);
  inputMgr->destroyInputObject(mouse);
  InputManager::destroyInputSystem(inputMgr);
  inputMgr = NULL;
}

void OISDriver::capture()
{
  // Capture keyboard and mouse events
  keyboard->capture();
  mouse->capture();
}

bool OISDriver::isDown(int index)
{
  // TODO: Extend to mouse buttons as well
  return keyboard->isKeyDown((OIS::KeyCode)index);
}

void OISDriver::adjustMouseClippingSize(int width, int height)
{
    const OIS::MouseState &ms = mouse->getMouseState();
    ms.width = width;
    ms.height = height;
}
