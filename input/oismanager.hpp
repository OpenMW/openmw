#ifndef _INPUT_OISMANAGER_H
#define _INPUT_OISMANAGER_H

#include "ogre/renderer.hpp"
#include <OIS/OIS.h>

namespace Input
{
  struct OISManager
  {
    OIS::InputManager *inputMgr;
    OIS::Mouse *mouse;
    OIS::Keyboard *keyboard;

    OISManager(Render::OgreRenderer &rend);
    ~OISManager();
  };
}
#endif
