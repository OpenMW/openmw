#ifndef GAME_MWBASE_MECHANICSMANAGER_H
#define GAME_MWBASE_MECHANICSMANAGER_H

#include <string>
#include <vector>

namespace Ogre
{
    class Vector3;
}

namespace ESM
{
    struct Class;
}

namespace MWWorld
{
    class Ptr;
    class CellStore;
}

namespace MWBase
{
    /// \brief Interface for game mechanics manager (implemented in MWMechanics)
    class MechanicsManager
    {
            MechanicsManager (const MechanicsManager&);
            ///< not implemented

            MechanicsManager& operator= (const MechanicsManager&);
            ///< not implemented

        public:

            MechanicsManager() {}

            virtual ~MechanicsManager() {}

            virtual void addActor (const MWWorld::Ptr& ptr) = 0;
            ///< Register an actor for stats management

            virtual void removeActor (const MWWorld::Ptr& ptr) = 0;
            ///< Deregister an actor for stats management

            virtual void dropActors (const MWWorld::CellStore *cellStore) = 0;
            ///< Deregister all actors in the given cell.

            virtual void watchActor (const MWWorld::Ptr& ptr) = 0;
            ///< On each update look for changes in a previously registered actor and update the
            /// GUI accordingly.

            virtual void update (std::vector<std::pair<std::string, Ogre::Vector3> >& movement,
                float duration, bool paused) = 0;
            ///< Update actor stats and store desired velocity vectors in \a movement
            ///
            /// \param paused In game type does not currently advance (this usually means some GUI
            /// component is up).

            virtual void setPlayerName (const std::string& name) = 0;
            ///< Set player name.

            virtual void setPlayerRace (const std::string& id, bool male) = 0;
            ///< Set player race.

            virtual void setPlayerBirthsign (const std::string& id) = 0;
            ///< Set player birthsign.

            virtual void setPlayerClass (const std::string& id) = 0;
            ///< Set player class to stock class.

            virtual void setPlayerClass (const ESM::Class& class_) = 0;
            ///< Set player class to custom class.

            virtual void restoreDynamicStats() = 0;
            ///< If the player is sleeping, this should be called every hour.

            virtual int barterOffer(const MWWorld::Ptr& ptr,int basePrice, bool buying) = 0;
            ///< This is used by every service to determine the price of objects given the trading skills of the player and NPC.

            virtual int disposition(const MWWorld::Ptr& ptr) = 0;
            ///< Calculate the diposition of an NPC toward the player.
    };
}

#endif
