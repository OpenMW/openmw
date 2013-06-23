#ifndef GAME_MWWORLD_PLAYER_H
#define GAME_MWWORLD_PLAYER_H

#include "OgreCamera.h"

#include <components/esm_store/cell_store.hpp>

#include "../mwworld/refdata.hpp"
#include "../mwworld/ptr.hpp"

namespace MWRender
{
    class Player;
}

namespace MWWorld
{
    class World;

    /// \brief NPC object representing the player and additional player data
    class Player
    {
        ESMS::LiveCellRef<ESM::NPC, MWWorld::RefData> mPlayer;
        MWWorld::Ptr::CellStore *mCellStore;
        MWRender::Player *mRenderer;
        MWWorld::World& mWorld;
        std::string mName;
        bool mMale;
        std::string mRace;
        std::string mBirthsign;
        ESM::Class *mClass;
        bool mCollisionMode;

    public:

        Player(MWRender::Player *renderer, const ESM::NPC *player, MWWorld::World& world);

        ~Player();

        /// Set the player position. Uses Morrowind coordinates.
        void setPos(float _x, float _y, float _z, bool updateCamera = false);

        void setCell (MWWorld::Ptr::CellStore *cellStore)
        {
            mCellStore = cellStore;
        }

        /// Move the player relative to her own position and
        /// orientation. After the call, the new position is returned.
        void moveRel (float &relX, float &relY, float &relZ);

        MWWorld::Ptr getPlayer()
        {
            MWWorld::Ptr ptr (&mPlayer, mCellStore);
            return ptr;
        }

        MWRender::Player *getRenderer() { return mRenderer; }

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

        void toggleCollisionMode()
        {
            mCollisionMode = !mCollisionMode;
        }
  };
}
#endif
