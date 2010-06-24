#ifndef _MWRENDER_PLAYERPOS_H
#define _MWRENDER_PLAYERPOS_H

#include "OgreCamera.h"

namespace MWRender
{
  // This class keeps track of the player position. It takes care of
  // camera movement, sound listener updates, and collision handling
  // (to be done).
  class PlayerPos
  {
    float x, y, z;
    Ogre::Camera *camera;

  public:
    PlayerPos(Ogre::Camera *cam) :
      camera(cam), x(0), y(0), z(0) {}

    // Set the player position. Uses Morrowind coordinates.
    void setPos(float _x, float _y, float _z)
    {
      x = _x;
      y = _y;
      z = _z;

      // TODO: Update sound listener
    }

    Ogre::Camera *getCamera() { return camera; }

    // Move the player relative to her own position and
    // orientation. After the call, the new position is returned.
    void moveRel(float &relX, float &relY, float &relZ)
    {
      using namespace Ogre;

      // Move camera relative to its own direction
      camera->moveRelative(Vector3(relX,0,relZ));

      // Up/down movement is always done relative the world axis.
      camera->move(Vector3(0,relY,0));

      // Get new camera position, converting back to MW coords.
      Vector3 pos = camera->getPosition();
      relX = pos[0];
      relY = -pos[2];
      relZ = pos[1];

      // TODO: Collision detection must be used to find the REAL new
      // position.

      // Set the position
      setPos(relX, relY, relZ);
    }
  };
}
#endif
