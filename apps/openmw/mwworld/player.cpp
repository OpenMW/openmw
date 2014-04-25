
#include "player.hpp"

#include <stdexcept>

#include <components/esm/esmreader.hpp>
#include <components/esm/esmwriter.hpp>
#include <components/esm/player.hpp>
#include <components/esm/defs.hpp>
#include <components/esm/loadbsgn.hpp>

#include "../mwworld/esmstore.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/soundmanager.hpp"

#include "../mwmechanics/movement.hpp"
#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/actors.hpp"
#include "../mwmechanics/mechanicsmanagerimp.hpp"

#include "class.hpp"
#include "ptr.hpp"
#include "inventorystore.hpp"
#include "cellstore.hpp"

namespace MWWorld
{
    Player::Player (const ESM::NPC *player, const MWBase::World& world)
      : mCellStore(0),
        mLastKnownExteriorPosition(0,0,0),
        mAutoMove(false),
        mForwardBackward(0),
        mTeleported(false),
        mMarkedCell(NULL),
        mCurrentCrimeId(-1),
        mPayedCrimeId(-1)
    {
        mPlayer.mBase = player;
        mPlayer.mRef.mRefID = "player";

        float* playerPos = mPlayer.mData.getPosition().pos;
        playerPos[0] = playerPos[1] = playerPos[2] = 0;
    }

    void Player::set(const ESM::NPC *player)
    {
        mPlayer.mBase = player;
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
        ptr.getClass().getCreatureStats(ptr).setMovementFlag(MWMechanics::CreatureStats::Flag_Run, run);
    }

    void Player::setSneak(bool sneak)
    {
        MWWorld::Ptr ptr = getPlayer();

        ptr.getClass().getCreatureStats(ptr).setMovementFlag(MWMechanics::CreatureStats::Flag_Sneak, sneak);

        if (sneak == true)
        {
            const MWWorld::ESMStore& esmStore = MWBase::Environment::get().getWorld()->getStore();

            // Find all the actors who might be able to see the player
            std::vector<MWWorld::Ptr> neighbors;
            MWBase::Environment::get().getMechanicsManager()->getActorsInRange( Ogre::Vector3(ptr.getRefData().getPosition().pos), 
                                        esmStore.get<ESM::GameSetting>().find("fSneakUseDist")->getInt(), neighbors);
            for (std::vector<MWWorld::Ptr>::iterator it = neighbors.begin(); it != neighbors.end(); ++it)
            {
                if ( MWBase::Environment::get().getMechanicsManager()->awarenessCheck(ptr, *it) )
                { 
                    MWBase::Environment::get().getWindowManager()->setSneakVisibility(false);
                    break;
                }
            }
            if (neighbors.size() == 0)
                MWBase::Environment::get().getWindowManager()->setSneakVisibility(true);
        }
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

    MWMechanics::DrawState_ Player::getDrawState()
    {
         MWWorld::Ptr ptr = getPlayer();
         return MWWorld::Class::get(ptr).getNpcStats(ptr).getDrawState();
    }

    bool Player::wasTeleported() const
    {
        return mTeleported;
    }

    void Player::setTeleported(bool teleported)
    {
        mTeleported = teleported;
    }

    void Player::markPosition(CellStore *markedCell, ESM::Position markedPosition)
    {
        mMarkedCell = markedCell;
        mMarkedPosition = markedPosition;
    }

    void Player::getMarkedPosition(CellStore*& markedCell, ESM::Position &markedPosition) const
    {
        markedCell = mMarkedCell;
        if (mMarkedCell)
            markedPosition = mMarkedPosition;
    }

    void Player::clear()
    {
        mCellStore = 0;
        mSign.clear();
        mMarkedCell = 0;
        mAutoMove = false;
        mForwardBackward = 0;
        mTeleported = false;
    }

    void Player::write (ESM::ESMWriter& writer) const
    {
        ESM::Player player;

        mPlayer.save (player.mObject);
        player.mCellId = mCellStore->getCell()->getCellId();

        player.mCurrentCrimeId = mCurrentCrimeId;
        player.mPayedCrimeId = mPayedCrimeId;

        player.mBirthsign = mSign;

        player.mLastKnownExteriorPosition[0] = mLastKnownExteriorPosition.x;
        player.mLastKnownExteriorPosition[1] = mLastKnownExteriorPosition.y;
        player.mLastKnownExteriorPosition[2] = mLastKnownExteriorPosition.z;

        if (mMarkedCell)
        {
            player.mHasMark = true;
            player.mMarkedPosition = mMarkedPosition;
            player.mMarkedCell = mMarkedCell->getCell()->getCellId();
        }
        else
            player.mHasMark = false;

        player.mAutoMove = mAutoMove ? 1 : 0;

        writer.startRecord (ESM::REC_PLAY);
        player.save (writer);
        writer.endRecord (ESM::REC_PLAY);
    }

    bool Player::readRecord (ESM::ESMReader& reader, int32_t type)
    {
        if (type==ESM::REC_PLAY)
        {
            ESM::Player player;
            player.load (reader);

            if (!mPlayer.checkState (player.mObject))
            {
                // this is the one object we can not silently drop.
                throw std::runtime_error ("invalid player state record (object state)");
            }

            mPlayer.load (player.mObject);

            MWBase::World& world = *MWBase::Environment::get().getWorld();

            mCellStore = world.getCell (player.mCellId);

            if (!player.mBirthsign.empty() &&
                !world.getStore().get<ESM::BirthSign>().search (player.mBirthsign))
                throw std::runtime_error ("invalid player state record (birthsign)");

            mCurrentCrimeId = player.mCurrentCrimeId;
            mPayedCrimeId = player.mPayedCrimeId;

            mSign = player.mBirthsign;

            mLastKnownExteriorPosition.x = player.mLastKnownExteriorPosition[0];
            mLastKnownExteriorPosition.y = player.mLastKnownExteriorPosition[1];
            mLastKnownExteriorPosition.z = player.mLastKnownExteriorPosition[2];

            if (player.mHasMark && !player.mMarkedCell.mPaged)
            {
                // interior cell -> need to check if it exists (exterior cell will be
                // generated on the fly)

                if (!world.getStore().get<ESM::Cell>().search (player.mMarkedCell.mWorldspace))
                    player.mHasMark = false; // drop mark silently
            }

            if (player.mHasMark)
            {
                mMarkedPosition = player.mMarkedPosition;
                mMarkedCell = world.getCell (player.mMarkedCell);
            }
            else
            {
                mMarkedCell = 0;
            }

            mAutoMove = player.mAutoMove!=0;

            mForwardBackward = 0;
            mTeleported = false;

            return true;
        }

        return false;
    }

    int Player::getNewCrimeId()
    {
        return ++mCurrentCrimeId;
    }

    void Player::recordCrimeId()
    {
        mPayedCrimeId = mCurrentCrimeId;
    }

    int Player::getCrimeId() const
    {
        return mPayedCrimeId;
    }
}
