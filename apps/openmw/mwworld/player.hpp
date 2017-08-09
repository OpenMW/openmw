#ifndef GAME_MWWORLD_PLAYER_H
#define GAME_MWWORLD_PLAYER_H

#include "../mwworld/refdata.hpp"
#include "../mwworld/livecellref.hpp"

#include "../mwmechanics/drawstate.hpp"
#include "../mwmechanics/stat.hpp"

#include <components/esm/loadskil.hpp>
#include <components/esm/attr.hpp>

namespace ESM
{
    struct NPC;
    class ESMWriter;
    class ESMReader;
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

        osg::Vec3f mLastKnownExteriorPosition;

        ESM::Position           mMarkedPosition;
        // If no position was marked, this is NULL
        CellStore*              mMarkedCell;

        bool                    mAutoMove;
        int                     mForwardBackward;
        bool                    mTeleported;

        int                     mCurrentCrimeId;    // the id assigned witnesses
        int                     mPaidCrimeId;      // the last id paid off (0 bounty)

        // Saved skills and attributes prior to becoming a werewolf
        MWMechanics::SkillValue mSaveSkills[ESM::Skill::Length];
        MWMechanics::AttributeValue mSaveAttributes[ESM::Attribute::Length];

        bool mAttackingOrSpell;

    public:

        Player(const ESM::NPC *player);

        void saveSkillsAttributes();
        void restoreSkillsAttributes();
        void setWerewolfSkillsAttributes();

        // For mark/recall magic effects
        void markPosition (CellStore* markedCell, const ESM::Position& markedPosition);
        void getMarkedPosition (CellStore*& markedCell, ESM::Position& markedPosition) const;

        /// Interiors can not always be mapped to a world position. However
        /// world position is still required for divine / almsivi magic effects
        /// and the player arrow on the global map.
        void setLastKnownExteriorPosition (const osg::Vec3f& position) { mLastKnownExteriorPosition = position; }
        osg::Vec3f getLastKnownExteriorPosition() const { return mLastKnownExteriorPosition; }

        void set (const ESM::NPC *player);

        void setCell (MWWorld::CellStore *cellStore);

        MWWorld::Ptr getPlayer();

        void setBirthSign(const std::string &sign);
        const std::string &getBirthSign() const;

        void setDrawState (MWMechanics::DrawState_ state);
        MWMechanics::DrawState_ getDrawState(); /// \todo constness

        /// Activate the object under the crosshair, if any
        void activate();

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

        void setAttackingOrSpell(bool attackingOrSpell);
        bool getAttackingOrSpell() const;

        ///Checks all nearby actors to see if anyone has an aipackage against you
        bool isInCombat();

        bool enemiesNearby();

        void clear();

        void write (ESM::ESMWriter& writer, Loading::Listener& progress) const;

        bool readRecord (ESM::ESMReader& reader, uint32_t type);

        int getNewCrimeId();  // get new id for witnesses
        void recordCrimeId(); // record the paid crime id when bounty is 0
        int getCrimeId() const;     // get the last paid crime id
    };
}
#endif
