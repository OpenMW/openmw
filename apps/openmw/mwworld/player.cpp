
#include "player.hpp"

#include "../mwrender/player.hpp"

#include "world.hpp"
#include "class.hpp"

namespace MWWorld
{
    Player::Player (MWRender::Player *renderer, const ESM::NPC *player, MWWorld::World& world) :
      mCellStore (0), mRenderer (renderer), mWorld (world), mClass (0),
      mAutoMove (false), mForwardBackward (0)
    {
        mPlayer.base = player;
        mName = player->name;
        mMale = !(player->flags & ESM::NPC::Female);
        mRace = player->race;
        mPlayer.ref.pos.pos[0] = mPlayer.ref.pos.pos[1] = mPlayer.ref.pos.pos[2] = 0;
        std::cout << renderer->getHandle();
        mPlayer.mData.setHandle (renderer->getHandle());
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
            mRenderer->getCamera()->setPosition (Ogre::Vector3 (
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

    void Player::setAutoMove (bool enable)
    {
        MWWorld::Ptr ptr = getPlayer();

        mAutoMove = enable;

        int value = mForwardBackward;

        if (mAutoMove)
            value = 1;

        MWWorld::Class::get (ptr).getMovementSettings (ptr).mForwardBackward = value;
    }

    void Player::setLeftRight (int value)
    {
        MWWorld::Ptr ptr = getPlayer();

        MWWorld::Class::get (ptr).getMovementSettings (ptr).mLeftRight = value;
    }

    void Player::setForwardBackward (int value)
    {
        MWWorld::Ptr ptr = getPlayer();

        mForwardBackward = value;

        if (mAutoMove)
            value = 1;

        MWWorld::Class::get (ptr).getMovementSettings (ptr).mForwardBackward = value;
    }

    void Player::toggleRunning()
    {
        MWWorld::Ptr ptr = getPlayer();

        bool running = MWWorld::Class::get (ptr).getStance (ptr, MWWorld::Class::Run, true);

        MWWorld::Class::get (ptr).setStance (ptr, MWWorld::Class::Run, !running);
    }
}
