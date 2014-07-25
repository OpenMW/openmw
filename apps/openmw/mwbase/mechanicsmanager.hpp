#ifndef GAME_MWBASE_MECHANICSMANAGER_H
#define GAME_MWBASE_MECHANICSMANAGER_H

#include <string>
#include <vector>
#include <list>
#include <stdint.h>

namespace Ogre
{
    class Vector3;
}

namespace ESM
{
    struct Class;

    class ESMReader;
    class ESMWriter;
}

namespace MWWorld
{
    class Ptr;
    class CellStore;
}

namespace Loading
{
    class Listener;
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

            virtual void advanceTime (float duration) = 0;

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

            virtual void rest(bool sleep) = 0;
            ///< If the player is sleeping or waiting, this should be called every hour.
            /// @param sleep is the player sleeping or waiting?

            virtual int getHoursToRest() const = 0;
            ///< Calculate how many hours the player needs to rest in order to be fully healed

            virtual int getBarterOffer(const MWWorld::Ptr& ptr,int basePrice, bool buying) = 0;
            ///< This is used by every service to determine the price of objects given the trading skills of the player and NPC.

            virtual int getDerivedDisposition(const MWWorld::Ptr& ptr) = 0;
            ///< Calculate the diposition of an NPC toward the player.

            virtual int countDeaths (const std::string& id) const = 0;
            ///< Return the number of deaths for actors with the given ID.

            /// Check if \a observer is potentially aware of \a ptr. Does not do a line of sight check!
            virtual bool awarenessCheck (const MWWorld::Ptr& ptr, const MWWorld::Ptr& observer) = 0;

            /// Makes \a ptr fight \a target. Also shouts a combat taunt.
            virtual void startCombat (const MWWorld::Ptr& ptr, const MWWorld::Ptr& target) = 0;

            enum OffenseType
            {
                OT_Theft, // Taking items owned by an NPC or a faction you are not a member of
                OT_Assault, // Attacking a peaceful NPC
                OT_Murder, // Murdering a peaceful NPC
                OT_Trespassing, // Staying in a cell you are not allowed in (where is this defined?)
                OT_SleepingInOwnedBed, // Sleeping in a bed owned by an NPC or a faction you are not a member of
                OT_Pickpocket // Entering pickpocket mode, leaving it, and being detected. Any items stolen are a separate crime (Theft)
            };
            /**
             * @brief Commit a crime. If any actors witness the crime and report it,
             *        reportCrime will be called automatically.
             * @note victim may be empty
             * @param arg Depends on \a type, e.g. for Theft, the value of the item that was stolen.
             * @return was the crime reported?
             */
            virtual bool commitCrime (const MWWorld::Ptr& ptr, const MWWorld::Ptr& victim,
                                      OffenseType type, int arg=0) = 0;
            virtual void reportCrime (const MWWorld::Ptr& ptr, const MWWorld::Ptr& victim,
                                      OffenseType type, int arg=0) = 0;
            /// Utility to check if taking this item is illegal and calling commitCrime if so
            virtual void itemTaken (const MWWorld::Ptr& ptr, const MWWorld::Ptr& item, int count) = 0;
            /// Utility to check if opening (i.e. unlocking) this object is illegal and calling commitCrime if so
            virtual void objectOpened (const MWWorld::Ptr& ptr, const MWWorld::Ptr& item) = 0;
            /// Attempt sleeping in a bed. If this is illegal, call commitCrime.
            /// @return was it illegal, and someone saw you doing it?
            virtual bool sleepInBed (const MWWorld::Ptr& ptr, const MWWorld::Ptr& bed) = 0;

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

            virtual void forceStateUpdate(const MWWorld::Ptr &ptr) = 0;
            ///< Forces an object to refresh its animation state.

            virtual void playAnimationGroup(const MWWorld::Ptr& ptr, const std::string& groupName, int mode, int number=1) = 0;
            ///< Run animation for a MW-reference. Calls to this function for references that are currently not
            /// in the scene should be ignored.
            ///
            /// \param mode 0 normal, 1 immediate start, 2 immediate loop
            /// \param count How many times the animation should be run

            virtual void skipAnimation(const MWWorld::Ptr& ptr) = 0;
            ///< Skip the animation for the given MW-reference for one frame. Calls to this function for
            /// references that are currently not in the scene should be ignored.

            virtual bool checkAnimationPlaying(const MWWorld::Ptr& ptr, const std::string& groupName) = 0;

            /// Update magic effects for an actor. Usually done automatically once per frame, but if we're currently
            /// paused we may want to do it manually (after equipping permanent enchantment)
            virtual void updateMagicEffects (const MWWorld::Ptr& ptr) = 0;

            virtual bool toggleAI() = 0;
            virtual bool isAIActive() = 0;

            virtual void getObjectsInRange (const Ogre::Vector3& position, float radius, std::vector<MWWorld::Ptr>& objects) = 0;
            virtual void getActorsInRange(const Ogre::Vector3 &position, float radius, std::vector<MWWorld::Ptr> &objects) = 0;

            ///return the list of actors which are following the given actor
            /**ie AiFollow is active and the target is the actor**/
            virtual std::list<MWWorld::Ptr> getActorsFollowing(const MWWorld::Ptr& actor) = 0;

            ///Returns a list of actors who are fighting the given actor within the fAlarmDistance
            /** ie AiCombat is active and the target is the actor **/
            virtual std::list<MWWorld::Ptr> getActorsFighting(const MWWorld::Ptr& actor) = 0;

            virtual void playerLoaded() = 0;

            virtual int countSavedGameRecords() const = 0;

            virtual void write (ESM::ESMWriter& writer, Loading::Listener& listener) const = 0;

            virtual void readRecord (ESM::ESMReader& reader, int32_t type) = 0;

            virtual void clear() = 0;

            /// @param bias Can be used to add an additional aggression bias towards the target,
            ///             making it more likely for the function to return true.
            virtual bool isAggressive (const MWWorld::Ptr& ptr, const MWWorld::Ptr& target, int bias=0, bool ignoreDistance=false) = 0;

            /// Usually done once a frame, but can be invoked manually in time-critical situations.
            /// This will increase the death count for any actors that were killed.
            virtual void killDeadActors() = 0;
    };
}

#endif
