
#include "player.hpp"


#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwmechanics/movement.hpp"
#include "../mwmechanics/npcstats.hpp"

#include "esmstore.hpp"
#include "class.hpp"

namespace MWWorld
{
    Player::Player (const ESM::NPC *player, const MWBase::World& world)
      : mCellStore(0),
        mClass(0),
        mRace(0),
        mSign(0),
        mAutoMove(false),
        mForwardBackward (0)
    {
        mPlayer.mBase = player;
        mPlayer.mRef.mRefID = "player";

        float* playerPos = mPlayer.mData.getPosition().pos;
        playerPos[0] = playerPos[1] = playerPos[2] = 0;

        mClass = world.getStore().get<ESM::Class>().find (player->mClass);
        mRace = world.getStore().get<ESM::Race>().find(player->mRace);
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

    void Player::setName(const std::string &value)
    {
        MWBase::Environment::get().getWorld()->updatePlayer(Data_Name, value);
    }

    void Player::setGender(bool value)
    {
        MWBase::Environment::get().getWorld()->updatePlayer(Data_Male, value);
    }

    void Player::setRace(const std::string &value)
    {
        MWBase::World *world = MWBase::Environment::get().getWorld();
        world->updatePlayer(Data_Race, value);

        mRace = world->getStore().get<ESM::Race>().find(value);
    }

    void Player::setBirthsign(const std::string &value)
    {
        MWBase::World *world = MWBase::Environment::get().getWorld();
        mSign = world->getStore().get<ESM::BirthSign>().find(value);
    }

    void Player::setClass(const std::string &value)
    {
        MWBase::World *world = MWBase::Environment::get().getWorld();
        world->updatePlayer(Data_Class, value);

        mClass = world->getStore().get<ESM::Class>().find(value);
    }
}
