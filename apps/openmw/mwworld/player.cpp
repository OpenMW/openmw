
#include "player.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/soundmanager.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/inventorystore.hpp"

#include "../mwmechanics/movement.hpp"
#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/character.hpp"
#include "../mwmechanics/security.hpp"

#include "../mwrender/animation.hpp"

#include "class.hpp"

namespace MWWorld
{
    Player::Player (const ESM::NPC *player, const MWBase::World& world)
      : mCellStore(0),
        mAutoMove(false),
        mForwardBackward (0)
    {
        mPlayer.mBase = player;
        mPlayer.mRef.mRefID = "player";

        float* playerPos = mPlayer.mData.getPosition().pos;
        playerPos[0] = playerPos[1] = playerPos[2] = 0;
    }

    void Player::set(const ESM::NPC *player)
    {
        mPlayer.mBase = player;

        float* playerPos = mPlayer.mData.getPosition().pos;
        playerPos[0] = playerPos[1] = playerPos[2] = 0;
    }

    void Player::setCell (MWWorld::CellStore *cellStore)
    {
        mCellStore = cellStore;
    }

    MWWorld::Ptr Player::getPlayer()
    {
        MWWorld::Ptr ptr (&mPlayer, mCellStore);
        return ptr;
    }

    void Player::setBirthSign (const std::string &sign)
    {
        mSign = sign;
    }

    const std::string& Player::getBirthSign() const
    {
        return mSign;
    }

    void Player::setDrawState (MWMechanics::DrawState_ state)
    {
         MWWorld::Ptr ptr = getPlayer();
         MWWorld::Class::get(ptr).getNpcStats(ptr).setDrawState (state);
    }

    bool Player::getAutoMove() const
    {
        return mAutoMove;
    }

    void Player::setAutoMove (bool enable)
    {
        MWWorld::Ptr ptr = getPlayer();

        mAutoMove = enable;

        int value = mForwardBackward;

        if (mAutoMove)
            value = 1;

        MWWorld::Class::get (ptr).getMovementSettings (ptr).mPosition[1] = value;
    }

    void Player::setLeftRight (int value)
    {
        MWWorld::Ptr ptr = getPlayer();

        MWWorld::Class::get (ptr).getMovementSettings (ptr).mPosition[0] = value;
    }

    void Player::setForwardBackward (int value)
    {
        MWWorld::Ptr ptr = getPlayer();

        mForwardBackward = value;

        if (mAutoMove)
            value = 1;

        MWWorld::Class::get (ptr).getMovementSettings (ptr).mPosition[1] = value;
    }

    void Player::setUpDown(int value)
    {
        MWWorld::Ptr ptr = getPlayer();

        MWWorld::Class::get (ptr).getMovementSettings (ptr).mPosition[2] = value;
    }

    void Player::setRunState(bool run)
    {
        MWWorld::Ptr ptr = getPlayer();
        MWWorld::Class::get(ptr).setStance(ptr, MWWorld::Class::Run, run);
    }

    void Player::setSneak(bool sneak)
    {
        MWWorld::Ptr ptr = getPlayer();

        MWWorld::Class::get (ptr).setStance (ptr, MWWorld::Class::Sneak, sneak);
    }

    void Player::yaw(float yaw)
    {
        MWWorld::Ptr ptr = getPlayer();
        MWWorld::Class::get(ptr).getMovementSettings(ptr).mRotation[2] += yaw;
    }
    void Player::pitch(float pitch)
    {
        MWWorld::Ptr ptr = getPlayer();
        MWWorld::Class::get(ptr).getMovementSettings(ptr).mRotation[0] += pitch;
    }
    void Player::roll(float roll)
    {
        MWWorld::Ptr ptr = getPlayer();
        MWWorld::Class::get(ptr).getMovementSettings(ptr).mRotation[1] += roll;
    }

    void Player::use()
    {
        MWWorld::InventoryStore& store = MWWorld::Class::get(getPlayer()).getInventoryStore(getPlayer());
        MWWorld::ContainerStoreIterator equipped = store.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);

        if (getDrawState() == MWMechanics::DrawState_Weapon)
        {
            if (equipped != store.end())
            {
                MWWorld::Ptr item = *equipped;
                MWRender::Animation* anim = MWBase::Environment::get().getWorld()->getAnimation(getPlayer());
                MWWorld::Ptr target = MWBase::Environment::get().getWorld()->getFacedObject();

                if (anim->isPriorityActive(MWMechanics::Priority_Weapon))
                    return;

                std::string resultMessage, resultSound;

                if (item.getTypeName() == typeid(ESM::Lockpick).name())
                {
                    if (!target.isEmpty())
                        MWMechanics::Security(getPlayer()).pickLock(target, item, resultMessage, resultSound);
                    anim->play("pickprobe", MWMechanics::Priority_Weapon, MWRender::Animation::Group_UpperBody, true, 1.0f, "start", "stop", 0.0, 0);
                }
                else if (item.getTypeName() == typeid(ESM::Probe).name())
                {
                    if (!target.isEmpty())
                        MWMechanics::Security(getPlayer()).probeTrap(target, item, resultMessage, resultSound);
                    anim->play("pickprobe", MWMechanics::Priority_Weapon, MWRender::Animation::Group_UpperBody, true, 1.0f, "start", "stop", 0.0, 0);
                }

                if (!resultMessage.empty())
                    MWBase::Environment::get().getWindowManager()->messageBox(resultMessage);
                if (!resultSound.empty())
                    MWBase::Environment::get().getSoundManager()->playSound(resultSound,1,1);

                // tool used up?
                if (!item.getRefData().getCount())
                    MWBase::Environment::get().getWindowManager()->unsetSelectedWeapon();
                else
                    MWBase::Environment::get().getWindowManager()->setSelectedWeapon(item);
            }
        }
    }

    MWMechanics::DrawState_ Player::getDrawState()
    {
         MWWorld::Ptr ptr = getPlayer();
         return MWWorld::Class::get(ptr).getNpcStats(ptr).getDrawState();
    }
}
