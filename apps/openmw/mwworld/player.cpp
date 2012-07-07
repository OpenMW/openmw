
#include "player.hpp"

#include <components/esm_store/store.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwrender/player.hpp"

#include "../mwmechanics/movement.hpp"
#include "../mwmechanics/npcstats.hpp"

#include "class.hpp"

namespace MWWorld
{
    Player::Player (MWRender::Player *renderer, const ESM::NPC *player, const MWBase::World& world) :
      mCellStore (0), mRenderer (renderer), mClass (0),
      mAutoMove (false), mForwardBackward (0)
    {
        mPlayer.base = player;
        mPlayer.ref.refID = "player";
        mName = player->name;
        mMale = !(player->flags & ESM::NPC::Female);
        mRace = player->race;

        float* playerPos = mPlayer.mData.getPosition().pos;
        playerPos[0] = playerPos[1] = playerPos[2] = 0;

        mPlayer.mData.setBaseNode(renderer->getNode());
        /// \todo Do not make a copy of classes defined in esm/p records.
        mClass = new ESM::Class (*world.getStore().classes.find (player->cls));
    }

    Player::~Player()
    {
        delete mClass;
    }

    void Player::setPos(float x, float y, float z)
    {
        /// \todo This fcuntion should be removed during the mwrender-refactoring.
        MWBase::Environment::get().getWorld()->moveObject (getPlayer(), x, y, z);
    }

    void Player::setRot(float x, float y, float z)
    {
        mRenderer->setRot(x, y, z);
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
