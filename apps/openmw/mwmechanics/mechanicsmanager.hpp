#ifndef GAME_MWMECHANICS_MECHANICSMANAGER_H
#define GAME_MWMECHANICS_MECHANICSMANAGER_H

#include <vector>
#include <string>

#include "../mwworld/ptr.hpp"

#include "creaturestats.hpp"
#include "npcstats.hpp"
#include "actors.hpp"

namespace Ogre
{
    class Vector3;
}

namespace MWMechanics
{
    class MechanicsManager
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

            void adjustMagicEffects (MWWorld::Ptr& creature);

        public:

            MechanicsManager ();

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

            void update (std::vector<std::pair<std::string, Ogre::Vector3> >& movement, float duration,
                bool paused);
            ///< Update actor stats and store desired velocity vectors in \a movement
            ///
            /// \param paused In game type does not currently advance (this usually means some GUI
            /// component is up).

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
