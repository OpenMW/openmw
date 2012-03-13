#ifndef MANGLE_INPUT_OGREINPUTFRAME_H
#define MANGLE_INPUT_OGREINPUTFRAME_H

/*
  This Ogre FrameListener calls capture() on an input driver every frame.
 */

#include <OgreFrameListener.h>
#include "../driver.hpp"

namespace Mangle {
namespace Input {

  struct OgreInputCapture : Ogre::FrameListener
  {
    Mangle::Input::Driver &driver;

    OgreInputCapture(Mangle::Input::Driver &drv)
      : driver(drv) {}

    bool frameStarted(const Ogre::FrameEvent &evt)
    {
      driver.capture();
      return true;
    }
  };
}}

#endif
