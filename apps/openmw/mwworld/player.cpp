
#include "player.hpp"

#include <components/esm_store/store.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwmechanics/movement.hpp"
#include "../mwmechanics/npcstats.hpp"

#include "class.hpp"

namespace MWWorld
{
    Player::Player (const ESM::NPC *player, const MWBase::World& world) :
      mCellStore (0), mClass (0),
      mAutoMove (false), mForwardBackward (0)
    {
        mPlayer.base = player;
        mPlayer.ref.mRefID = "player";
        mName = player->mName;
        mMale = !(player->mFlags & ESM::NPC::Female);
        mRace = player->mRace;

        float* playerPos = mPlayer.mData.getPosition().pos;
        playerPos[0] = playerPos[1] = playerPos[2] = 0;

        /// \todo Do not make a copy of classes defined in esm/p records.
        mClass = new ESM::Class (*world.getStore().classes.find (player->mClass));
    }

    Player::~Player()
    {
        delete mClass;
    }

    void Player::setClass (const ESM::Class& class_)
    {
        ESM::Class *new_class = new ESM::Class (class_);
        delete mClass;
        mClass = new_class;
    }

    void Player::setDrawState (MWMechanics::DrawState_ state)
    {
         MWWorld::Ptr ptr = getPlayer();
         MWWorld::Class::get(ptr).getNpcStats(ptr).setDrawState (state);
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

    void Player::setUpDown(int value)
    {
        MWWorld::Ptr ptr = getPlayer();

        MWWorld::Class::get (ptr).getMovementSettings (ptr).mUpDown = value;
    }

    void Player::toggleRunning()
    {
        MWWorld::Ptr ptr = getPlayer();

        bool running = MWWorld::Class::get (ptr).getStance (ptr, MWWorld::Class::Run, true);

        MWWorld::Class::get (ptr).setStance (ptr, MWWorld::Class::Run, !running);
    }

    MWMechanics::DrawState_ Player::getDrawState()
    {
         MWWorld::Ptr ptr = getPlayer();
         return MWWorld::Class::get(ptr).getNpcStats(ptr).getDrawState();
    }

}
