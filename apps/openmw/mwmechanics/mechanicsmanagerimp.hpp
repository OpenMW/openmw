#ifndef GAME_MWMECHANICS_MECHANICSMANAGERIMP_H
#define GAME_MWMECHANICS_MECHANICSMANAGERIMP_H

#include "../mwbase/mechanicsmanager.hpp"

#include "../mwworld/ptr.hpp"

#include "creaturestats.hpp"
#include "npcstats.hpp"
#include "objects.hpp"
#include "actors.hpp"

namespace Ogre
{
    class Vector3;
}

namespace MWWorld
{
    class CellStore;
}

namespace MWMechanics
{
    class MechanicsManager : public MWBase::MechanicsManager
    {
            MWWorld::Ptr mWatched;
            CreatureStats mWatchedCreature;
            NpcStats mWatchedNpc;
            bool mUpdatePlayer;
            bool mClassSelected;
            bool mRaceSelected;

            Objects mObjects;
            Actors mActors;

            void buildPlayer();
            ///< build player according to stored class/race/birthsign information. Will
            /// default to the values of the ESM::NPC object, if no explicit information is given.

        public:

            MechanicsManager();

            virtual void add (const MWWorld::Ptr& ptr);
            ///< Register an object for management

            virtual void remove (const MWWorld::Ptr& ptr);
            ///< Deregister an object for management

            virtual void updateCell(const MWWorld::Ptr &old, const MWWorld::Ptr &ptr);
            ///< Moves an object to a new cell

            virtual void drop(const MWWorld::CellStore *cellStore);
            ///< Deregister all objects in the given cell.

            virtual void watchActor(const MWWorld::Ptr& ptr);
            ///< On each update look for changes in a previously registered actor and update the
            /// GUI accordingly.

            virtual void update (float duration, bool paused);
            ///< Update objects
            ///
            /// \param paused In game type does not currently advance (this usually means some GUI
            /// component is up).

            virtual void setPlayerName (const std::string& name);
            ///< Set player name.

            virtual void setPlayerRace (const std::string& id, bool male, const std::string &head, const std::string &hair);
            ///< Set player race.

            virtual void setPlayerBirthsign (const std::string& id);
            ///< Set player birthsign.

            virtual void setPlayerClass (const std::string& id);
            ///< Set player class to stock class.

            virtual void setPlayerClass (const ESM::Class& class_);
            ///< Set player class to custom class.

            virtual void restoreDynamicStats();
            ///< If the player is sleeping, this should be called every hour.

            virtual int getBarterOffer(const MWWorld::Ptr& ptr,int basePrice, bool buying);
            ///< This is used by every service to determine the price of objects given the trading skills of the player and NPC.

            virtual int getDerivedDisposition(const MWWorld::Ptr& ptr);
            ///< Calculate the diposition of an NPC toward the player.

            virtual int countDeaths (const std::string& id) const;
            ///< Return the number of deaths for actors with the given ID.
            
            virtual void getPersuasionDispositionChange (const MWWorld::Ptr& npc, PersuasionType type,
                float currentTemporaryDispositionDelta, bool& success, float& tempChange, float& permChange);
            void toLower(std::string npcFaction);
            ///< Perform a persuasion action on NPC

        virtual void playAnimationGroup(const MWWorld::Ptr& ptr, const std::string& groupName, int mode, int number);
        virtual void skipAnimation(const MWWorld::Ptr& ptr);
    };
}

#endif
