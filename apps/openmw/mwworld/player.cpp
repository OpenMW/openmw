
#include "player.hpp"

#include "../mwrender/player.hpp"

#include "world.hpp"

namespace MWWorld
{
    Player::Player (MWRender::Player *renderer, const ESM::NPC *player, MWWorld::World& world) :
      mCellStore (0), mRenderer (renderer), mWorld (world), mClass (0), mCollisionMode (true)
    {
        mPlayer.base = player;
        mName = player->name;
        mMale = !(player->flags & ESM::NPC::Female);
        mRace = player->race;
        mPlayer.ref.pos.pos[0] = mPlayer.ref.pos.pos[1] = mPlayer.ref.pos.pos[2] = 0;
        mClass = new ESM::Class (*world.getStore().classes.find (player->cls));
	mAutoMove = false;
	misWalking = false;
    }

    Player::~Player()
    {
        delete mClass;
    }

    void Player::setPos(float x, float y, float z, bool updateCamera)
    {
        mWorld.moveObject (getPlayer(), x, y, z);

        if (updateCamera)
            mRenderer->getCamera()->setPosition (Ogre::Vector3 (
                mPlayer.ref.pos.pos[0],
                mPlayer.ref.pos.pos[2],
                -mPlayer.ref.pos.pos[1]));
    }

    void Player::moveRel (float &relX, float &relY, float &relZ)
    {
        // Move camera relative to its own direction
        mRenderer->getCamera()->moveRelative (Ogre::Vector3(relX,0,relZ));

        // Up/down movement is always done relative the world axis.
        mRenderer->getCamera()->move (Ogre::Vector3(0,relY,0));

        // Get new camera position, converting back to MW coords.
        Ogre::Vector3 pos = mRenderer->getCamera()->getPosition();
        relX = pos[0];
        relY = -pos[2];
        relZ = pos[1];

        // TODO: Collision detection must be used to find the REAL new
        // position, if mCollisionMode==true

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
