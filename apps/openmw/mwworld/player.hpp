#ifndef GAME_MWWORLD_PLAYER_H
#define GAME_MWWORLD_PLAYER_H

#include "OgreCamera.h"

#include "../mwworld/cellstore.hpp"
#include "../mwworld/refdata.hpp"
#include "../mwworld/ptr.hpp"

#include "../mwmechanics/drawstate.hpp"

namespace MWBase
{
    class World;
}

namespace MWWorld
{
    class CellStore;

    /// \brief NPC object representing the player and additional player data
    class Player
    {
        LiveCellRef<ESM::NPC> mPlayer;
        MWWorld::CellStore *mCellStore;
        std::string mName;
        bool mMale;
        std::string mRace;
        std::string mBirthsign;
        ESM::Class *mClass;
        bool mAutoMove;
        int mForwardBackward;
    public:

        Player(const ESM::NPC *player, const MWBase::World& world);

        ~Player();

        void setCell (MWWorld::CellStore *cellStore)
        {
            mCellStore = cellStore;
        }

        MWWorld::Ptr getPlayer()
        {
            MWWorld::Ptr ptr (&mPlayer, mCellStore);
            return ptr;
        }

        void setName (const std::string& name)
        {
            mName = name;
        }

        void setGender (bool male)
        {
            mMale = male;
        }

        void setRace (const std::string& race)
        {
            mRace = race;
        }

        void setBirthsign (const std::string& birthsign)
        {
            mBirthsign = birthsign;
        }

        void setClass (const ESM::Class& class_);

        void setDrawState (MWMechanics::DrawState_ state);

        std::string getName() const
        {
            return mName;
        }

        bool isMale() const
        {
            return mMale;
        }

        std::string getRace() const
        {
            return mRace;
        }

        std::string getBirthsign() const
        {
            return mBirthsign;
        }

        const ESM::Class& getClass() const
        {
            return *mClass;
        }

        bool getAutoMove() const
        {
            return mAutoMove;
        }

        MWMechanics::DrawState_ getDrawState(); /// \todo constness

        void setAutoMove (bool enable);

        void setLeftRight (int value);

        void setForwardBackward (int value);
        void setUpDown(int value);

        void toggleRunning();
    };
}
#endif
