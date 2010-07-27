#ifndef MANGLE_SOUND_OGRELISTENERMOVER_H
#define MANGLE_SOUND_OGRELISTENERMOVER_H

#include <OgreCamera.h>
#include <assert.h>
#include "../output.hpp"

namespace Mangle {
namespace Sound {

  /** This class lets a sound listener (ie. the SoundFactory) track a
      given camera in Ogre3D. The poisition and orientation of the
      listener will be updated to match the camera whenever the camera
      is moved.
   */
  struct OgreListenerMover : Ogre::Camera::Listener
  {
    OgreListenerMover(Mangle::Sound::SoundFactoryPtr snd)
      : soundFact(snd), camera(NULL)
    {}

    /// Follow a camera. WARNING: This will OVERRIDE any other
    /// MovableObject::Listener you may have attached to the camera.
    void followCamera(Ogre::Camera *cam)
    {
      camera = cam;
      camera->addListener(this);
    }

  private:
    Mangle::Sound::SoundFactoryPtr soundFact;
    Ogre::Camera *camera;
    Ogre::Vector3 pos, dir, up;

    /// From Camera::Listener. This is called once per
    /// frame. Unfortunately, Ogre doesn't allow us to be notified
    /// only when the camera itself has moved, so we must poll every
    /// frame.
    void cameraPreRenderScene(Ogre::Camera *cam)
    {
      assert(cam == camera);

      Ogre::Vector3 nPos, nDir, nUp;

      nPos = camera->getPosition();
      nDir = camera->getDirection();
      nUp  = camera->getUp();

      // Don't bother the sound system needlessly
      if(nDir != dir || nPos != pos || nUp != up)
        {
          pos = nPos;
          dir = nDir;
          up  = nUp;

          soundFact->setListenerPos(pos.x, pos.y, pos.z,
                                    dir.x, dir.y, dir.z,
                                    up.x, up.y, up.z);
        }
    }

    void cameraDestroyed(Ogre::Camera *cam)
    {
      assert(cam == camera);
      camera = NULL;
    }
  };
}}
#endif
