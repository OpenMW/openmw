#ifndef GAME_MWMECHANICS_MECHANICSMANAGERIMP_H
#define GAME_MWMECHANICS_MECHANICSMANAGERIMP_H

#include "../mwbase/mechanicsmanager.hpp"

#include "../mwworld/ptr.hpp"

#include "creaturestats.hpp"
#include "npcstats.hpp"
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
            Actors mActors;

            void buildPlayer();
            ///< build player according to stored class/race/birthsign information. Will
            /// default to the values of the ESM::NPC object, if no explicit information is given.

        public:

            MechanicsManager();

            virtual void addActor (const MWWorld::Ptr& ptr);
            ///< Register an actor for stats management

            virtual void removeActor (const MWWorld::Ptr& ptr);
            ///< Deregister an actor for stats management

            virtual void dropActors (const MWWorld::CellStore *cellStore);
            ///< Deregister all actors in the given cell.

            virtual void watchActor (const MWWorld::Ptr& ptr);
            ///< On each update look for changes in a previously registered actor and update the
            /// GUI accordingly.

            virtual void update (std::vector<std::pair<std::string, Ogre::Vector3> >& movement,
                float duration, bool paused);
            ///< Update actor stats and store desired velocity vectors in \a movement
            ///
            /// \param paused In game type does not currently advance (this usually means some GUI
            /// component is up).

            virtual void setPlayerName (const std::string& name);
            ///< Set player name.

            virtual void setPlayerRace (const std::string& id, bool male);
            ///< Set player race.

            virtual void setPlayerBirthsign (const std::string& id);
            ///< Set player birthsign.

            virtual void setPlayerClass (const std::string& id);
            ///< Set player class to stock class.

            virtual void setPlayerClass (const ESM::Class& class_);
            ///< Set player class to custom class.

            virtual void restoreDynamicStats();
            ///< If the player is sleeping, this should be called every hour.

            virtual int barterOffer(const MWWorld::Ptr& ptr,int basePrice, bool buying);
            ///< This is used by every service to determine the price of objects given the trading skills of the player and NPC.

            virtual int disposition(const MWWorld::Ptr& ptr);
            ///< Calculate the diposition of an NPC toward the player.
    };
}

#endif
