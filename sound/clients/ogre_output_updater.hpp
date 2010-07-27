#ifndef MANGLE_SOUND_OGREUPDATER_H
#define MANGLE_SOUND_OGREUPDATER_H

/*
  This Ogre FrameListener calls update on a SoundFactory
 */

#include <OgreFrameListener.h>
#include "../output.hpp"
#include <assert.h>

namespace Mangle {
namespace Sound {

  struct OgreOutputUpdater : Ogre::FrameListener
  {
    Mangle::Sound::SoundFactory &driver;

    OgreOutputUpdater(Mangle::Sound::SoundFactory &drv)
      : driver(drv)
    { assert(drv.needsUpdate); }

    bool frameStarted(const Ogre::FrameEvent &evt)
    {
      driver.update();
      return true;
    }
  };
}}

#endif
