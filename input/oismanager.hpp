#ifndef _INPUT_OISMANAGER_H
#define _INPUT_OISMANAGER_H

#include "ogre/renderer.hpp"
#include <OIS/OIS.h>

namespace Input
{
  class OISManager
  {
    OIS::InputManager *inputMgr;
    OIS::Mouse *mouse;
    OIS::Keyboard *keyboard;

  public:
    void setup(Render::OgreRenderer *rend);
    void cleanup();
  };
}
#endif
