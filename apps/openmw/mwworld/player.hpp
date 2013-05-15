#ifndef GAME_MWWORLD_PLAYER_H
#define GAME_MWWORLD_PLAYER_H

#include "../mwworld/refdata.hpp"
#include "../mwworld/livecellref.hpp"

#include "../mwmechanics/drawstate.hpp"

namespace ESM
{
    struct NPC;
}

namespace MWBase
{
    class World;
    class Ptr;
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

        bool                    mAutoMove;
        int                     mForwardBackward;

    public:

        Player(const ESM::NPC *player, const MWBase::World& world);

        void set (const ESM::NPC *player);

        void setCell (MWWorld::CellStore *cellStore);

        MWWorld::Ptr getPlayer();

        void setBirthSign(const std::string &sign);

        const std::string &getBirthSign() const;

        void setDrawState (MWMechanics::DrawState_ state);

        bool getAutoMove() const;

        MWMechanics::DrawState_ getDrawState(); /// \todo constness

        void setAutoMove (bool enable);

        void setLeftRight (int value);

        void setForwardBackward (int value);
        void setUpDown(int value);

        void setRunState(bool run);
        void setSneak(bool sneak);

        void setYaw(float yaw);
        void setPitch(float pitch);
        void setRoll(float roll);
    };
}
#endif
