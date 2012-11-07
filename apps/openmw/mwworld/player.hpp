#ifndef GAME_MWWORLD_PLAYER_H
#define GAME_MWWORLD_PLAYER_H

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
        LiveCellRef<ESM::NPC>   mPlayer;
        MWWorld::CellStore      *mCellStore;

        // cached referenced data
        const ESM::Class        *mClass;
        const ESM::Race         *mRace;
        const ESM::BirthSign    *mSign;

        bool                    mAutoMove;
        int                     mForwardBackward;

    public:
        enum {
            Data_Male,
            Data_Name,
            Data_Race,
            Data_Class,
            Data_Sign,
            Data_Model,
            Data_Head,
            Data_Hair
        };

        Player(const ESM::NPC *player, const MWBase::World& world);

        void setCell (MWWorld::CellStore *cellStore)
        {
            mCellStore = cellStore;
        }

        MWWorld::Ptr getPlayer()
        {
            MWWorld::Ptr ptr (&mPlayer, mCellStore);
            return ptr;
        }

        void setName (const std::string& name);
        void setGender (bool male);
        void setRace (const std::string& race);
        void setBirthsign (const std::string& birthsign);
        void setClass (const std::string &cls);

        void setDrawState (MWMechanics::DrawState_ state);

        std::string getName() const
        {
            return mPlayer.mBase->mName;
        }

        bool isMale() const
        {
            return (mPlayer.mBase->mFlags & 0x1) == 0;
        }

        const ESM::Race *getRace() const
        {
            return mRace;
        }

        const ESM::BirthSign *getBirthsign() const
        {
            return mSign;
        }

        const ESM::Class *getClass() const
        {
            return mClass;
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
