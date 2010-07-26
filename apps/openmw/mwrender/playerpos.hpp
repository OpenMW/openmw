#ifndef _MWRENDER_PLAYERPOS_H
#define _MWRENDER_PLAYERPOS_H

#include "OgreCamera.h"

#include <components/esm_store/cell_store.hpp>

#include "../mwworld/refdata.hpp"
#include "../mwworld/ptr.hpp"

namespace MWRender
{
  // This class keeps track of the player position. It takes care of
  // camera movement, sound listener updates, and collision handling
  // (to be done).
  class PlayerPos
  {
    ESMS::LiveCellRef<ESM::NPC, MWWorld::RefData> mPlayer;
    MWWorld::Ptr::CellStore *mCellStore;
    Ogre::Camera *camera;

  public:
    PlayerPos(Ogre::Camera *cam, const ESM::NPC *player) :
      mCellStore (0), camera(cam)
    {
        mPlayer.base = player;
        mPlayer.ref.pos.pos[0] = mPlayer.ref.pos.pos[1] = mPlayer.ref.pos.pos[2] = 0;
    }

    // Set the player position. Uses Morrowind coordinates.
    void setPos(float _x, float _y, float _z, bool updateCamera = false)
    {
      mPlayer.ref.pos.pos[0] = _x;
      mPlayer.ref.pos.pos[1] = _y;
      mPlayer.ref.pos.pos[2] = _z;

        if (updateCamera)
            camera->setPosition (Ogre::Vector3 (
                mPlayer.ref.pos.pos[0],
                mPlayer.ref.pos.pos[2],
                mPlayer.ref.pos.pos[1]));

      // TODO: Update sound listener
    }
    
    void setCell (MWWorld::Ptr::CellStore *cellStore)
    {
        mCellStore = cellStore;
    }

    Ogre::Camera *getCamera() { return camera; }

    // Move the player relative to her own position and
    // orientation. After the call, the new position is returned.
    void moveRel(float &relX, float &relY, float &relZ)
    {
      // TODO: Update mPlayer state
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
    
        MWWorld::Ptr getPlayer()
        {
            MWWorld::Ptr ptr (&mPlayer, mCellStore);
            return ptr;
        }
  };
}
#endif
