
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

    void Player::setClass (const ESM::Class& class_)
    {
        ESM::Class *new_class = new ESM::Class (class_);
        delete mClass;
        mClass = new_class;
    }
}
