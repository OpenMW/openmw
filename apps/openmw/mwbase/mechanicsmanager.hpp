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

            virtual void add (const MWWorld::Ptr& ptr) = 0;
            ///< Register an object for management

            virtual void remove (const MWWorld::Ptr& ptr) = 0;
            ///< Deregister an object for management

            virtual void updateCell(const MWWorld::Ptr &old, const MWWorld::Ptr &ptr) = 0;
            ///< Moves an object to a new cell

            virtual void drop (const MWWorld::CellStore *cellStore) = 0;
            ///< Deregister all objects in the given cell.

            virtual void watchActor (const MWWorld::Ptr& ptr) = 0;
            ///< On each update look for changes in a previously registered actor and update the
            /// GUI accordingly.

            virtual void update (float duration, bool paused) = 0;
            ///< Update objects
            ///
            /// \param paused In game type does not currently advance (this usually means some GUI
            /// component is up).

            virtual void setPlayerName (const std::string& name) = 0;
            ///< Set player name.

            virtual void setPlayerRace (const std::string& id, bool male, const std::string &head, const std::string &hair) = 0;
            ///< Set player race.

            virtual void setPlayerBirthsign (const std::string& id) = 0;
            ///< Set player birthsign.

            virtual void setPlayerClass (const std::string& id) = 0;
            ///< Set player class to stock class.

            virtual void setPlayerClass (const ESM::Class& class_) = 0;
            ///< Set player class to custom class.

            virtual void restoreDynamicStats() = 0;
            ///< If the player is sleeping, this should be called every hour.

            virtual int getBarterOffer(const MWWorld::Ptr& ptr,int basePrice, bool buying) = 0;
            ///< This is used by every service to determine the price of objects given the trading skills of the player and NPC.

            virtual int getDerivedDisposition(const MWWorld::Ptr& ptr) = 0;
            ///< Calculate the diposition of an NPC toward the player.

            virtual int countDeaths (const std::string& id) const = 0;
            ///< Return the number of deaths for actors with the given ID.

            enum PersuasionType
            {
                PT_Admire,
                PT_Intimidate,
                PT_Taunt,
                PT_Bribe10,
                PT_Bribe100,
                PT_Bribe1000
            };
            virtual void getPersuasionDispositionChange (const MWWorld::Ptr& npc, PersuasionType type,
                float currentTemporaryDispositionDelta, bool& success, float& tempChange, float& permChange) = 0;
            ///< Perform a persuasion action on NPC

        virtual void playAnimationGroup(const MWWorld::Ptr& ptr, const std::string& groupName, int mode, int number=1) = 0;
        ///< Run animation for a MW-reference. Calls to this function for references that are currently not
        /// in the scene should be ignored.
        ///
        /// \param mode 0 normal, 1 immediate start, 2 immediate loop
        /// \param count How many times the animation should be run

        virtual void skipAnimation(const MWWorld::Ptr& ptr) = 0;
        ///< Skip the animation for the given MW-reference for one frame. Calls to this function for
        /// references that are currently not in the scene should be ignored.
    };
}

#endif
