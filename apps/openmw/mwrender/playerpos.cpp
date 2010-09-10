
#include "playerpos.hpp"

#include "../mwworld/world.hpp"

namespace MWRender
{
    void PlayerPos::setPos(float x, float y, float z, bool updateCamera)
    {
        mWorld.moveObject (getPlayer(), x, y, z);

        if (updateCamera)
            camera->setPosition (Ogre::Vector3 (
                mPlayer.ref.pos.pos[0],
                mPlayer.ref.pos.pos[2],
                -mPlayer.ref.pos.pos[1]));
    }
}
