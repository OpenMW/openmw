#ifndef GAME_MWMECHANICS_MECHANICSMANAGER_H
#define GAME_MWMECHANICS_MECHANICSMANAGER_H

#include <set>

#include "../mwworld/ptr.hpp"

#include "creaturestats.hpp"

namespace MWWorld
{
    class Environment;
}

namespace MWMechanics
{
    class MechanicsManager
    {
            MWWorld::Environment& mEnvironment;
            std::set<MWWorld::Ptr> mActors;
            MWWorld::Ptr mWatched;
            CreatureStats mWatchedCreature;
            bool mUpdatePlayer;

            void buildPlayer();
            ///< build player according to stored class/race/birthsign information. Will
            /// default to the values of the ESM::NPC object, if no explicit information is given.

        public:

            MechanicsManager (MWWorld::Environment& environment);

            void configureGUI();

            void addActor (const MWWorld::Ptr& ptr);
            ///< Register an actor for stats management

            void removeActor (const MWWorld::Ptr& ptr);
            ///< Deregister an actor for stats management

            void dropActors (const MWWorld::Ptr::CellStore *cellStore);
            ///< Deregister all actors in the given cell.

            void watchActor (const MWWorld::Ptr& ptr);
            ///< On each update look for changes in a previously registered actor and update the
            /// GUI accordingly.

            void update();
            ///< Update actor stats

            void setPlayerName (const std::string& name);
            ///< Set player name.

            void setPlayerRace (const std::string& id, bool male);
            ///< Set player race.

            void setPlayerBirthsign (const std::string& id);
            ///< Set player birthsign.

            void setPlayerClass (const std::string& id);
            ///< Set player class to stock class.

            void setPlayerClass (const ESM::Class& class_);
            ///< Set player class to custom class.
    };
}

#endif
