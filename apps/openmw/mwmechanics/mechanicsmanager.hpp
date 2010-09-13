#ifndef GAME_MWMECHANICS_MECHANICSMANAGER_H
#define GAME_MWMECHANICS_MECHANICSMANAGER_H

#include <set>

#include "../mwworld/ptr.hpp"

#include "creaturestats.hpp"

namespace ESMS
{
    struct ESMStore;
}

namespace MWGui
{
    class WindowManager;
}

namespace MWMechanics
{
    class MechanicsManager
    {
            const ESMS::ESMStore& mStore;
            MWGui::WindowManager& mWindowManager;
            std::set<MWWorld::Ptr> mActors;
            MWWorld::Ptr mWatched;
            CreatureStats mWatchedCreature;

        public:

            MechanicsManager (const ESMS::ESMStore& store, MWGui::WindowManager& windowManager);

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

            void finalizePlayer();
            ///< Configure player according to previously called setPlayer functions.

    };
}

#endif
