
#include "player.hpp"

#include "world.hpp"

namespace MWWorld
{
    Player::Player (Ogre::Camera *cam, const ESM::NPC *player, MWWorld::World& world) :
      mCellStore (0), camera(cam), mWorld (world), mClass (0)
    {
        mPlayer.base = player;
        mName = player->name;
        mMale = !(player->flags & ESM::NPC::Female);
        mRace = player->race;
        mPlayer.ref.pos.pos[0] = mPlayer.ref.pos.pos[1] = mPlayer.ref.pos.pos[2] = 0;
        mClass = new ESM::Class (*world.getStore().classes.find (player->cls));
    }

    Player::~Player()
    {
        delete mClass;
    }

    void Player::setPos(float x, float y, float z, bool updateCamera)
    {
        mWorld.moveObject (getPlayer(), x, y, z);

        if (updateCamera)
            camera->setPosition (Ogre::Vector3 (
                mPlayer.ref.pos.pos[0],
                mPlayer.ref.pos.pos[2],
                -mPlayer.ref.pos.pos[1]));
    }

    void Player::moveRel (float &relX, float &relY, float &relZ)
    {
        // Move camera relative to its own direction
        camera->moveRelative (Ogre::Vector3(relX,0,relZ));

        // Up/down movement is always done relative the world axis.
        camera->move (Ogre::Vector3(0,relY,0));

        // Get new camera position, converting back to MW coords.
        Ogre::Vector3 pos = camera->getPosition();
        relX = pos[0];
        relY = -pos[2];
        relZ = pos[1];

        // TODO: Collision detection must be used to find the REAL new
        // position.

        // Set the position
        setPos(relX, relY, relZ);
    }

    void Player::setClass (const ESM::Class& class_)
    {
        ESM::Class *new_class = new ESM::Class (class_);
        delete mClass;
        mClass = new_class;
    }
}
