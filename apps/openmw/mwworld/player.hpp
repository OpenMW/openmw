#ifndef GAME_MWWORLD_PLAYER_H
#define GAME_MWWORLD_PLAYER_H

#include "../mwworld/refdata.hpp"
#include "../mwworld/livecellref.hpp"

#include "../mwmechanics/drawstate.hpp"

#include <OgreVector3.h>

namespace ESM
{
    struct NPC;
    class ESMWriter;
    class ESMReader;
}

namespace MWBase
{
    class World;
    class Ptr;
}

namespace Loading
{
    class Listener;
}

namespace MWWorld
{
    class CellStore;

    /// \brief NPC object representing the player and additional player data
    class Player
    {
        LiveCellRef<ESM::NPC>   mPlayer;
        MWWorld::CellStore      *mCellStore;
        std::string             mSign;

        Ogre::Vector3 mLastKnownExteriorPosition;

        ESM::Position           mMarkedPosition;
        // If no position was marked, this is NULL
        CellStore*              mMarkedCell;

        bool                    mAutoMove;
        int                     mForwardBackward;
        bool                    mTeleported;

        int                     mCurrentCrimeId;    // the id assigned witnesses
        int                     mPaidCrimeId;      // the last id paid off (0 bounty)

    public:

        Player(const ESM::NPC *player, const MWBase::World& world);

        // For mark/recall magic effects
        void markPosition (CellStore* markedCell, ESM::Position markedPosition);
        void getMarkedPosition (CellStore*& markedCell, ESM::Position& markedPosition) const;

        /// Interiors can not always be mapped to a world position. However
        /// world position is still required for divine / almsivi magic effects
        /// and the player arrow on the global map.
        /// TODO: This should be stored in the savegame, too.
        void setLastKnownExteriorPosition (const Ogre::Vector3& position) { mLastKnownExteriorPosition = position; }
        Ogre::Vector3 getLastKnownExteriorPosition() const { return mLastKnownExteriorPosition; }

        void set (const ESM::NPC *player);

        void setCell (MWWorld::CellStore *cellStore);

        MWWorld::Ptr getPlayer();

        void setBirthSign(const std::string &sign);
        const std::string &getBirthSign() const;

        void setDrawState (MWMechanics::DrawState_ state);
        MWMechanics::DrawState_ getDrawState(); /// \todo constness

        bool getAutoMove() const;
        void setAutoMove (bool enable);

        void setLeftRight (int value);

        void setForwardBackward (int value);
        void setUpDown(int value);

        void setRunState(bool run);
        void setSneak(bool sneak);

        void yaw(float yaw);
        void pitch(float pitch);
        void roll(float roll);

        bool wasTeleported() const;
        void setTeleported(bool teleported);

        ///Checks all nearby actors to see if anyone has an aipackage against you
        bool isInCombat();

        void clear();

        void write (ESM::ESMWriter& writer, Loading::Listener& progress) const;

        bool readRecord (ESM::ESMReader& reader, uint32_t type);

        int getNewCrimeId();  // get new id for witnesses
        void recordCrimeId(); // record the paid crime id when bounty is 0
        int getCrimeId() const;     // get the last paid crime id
    };
}
#endif
