#ifndef GAME_MWMECHANICS_MECHANICSMANAGERIMP_H
#define GAME_MWMECHANICS_MECHANICSMANAGERIMP_H

#include "../mwbase/mechanicsmanager.hpp"

#include "../mwworld/ptr.hpp"

#include "creaturestats.hpp"
#include "npcstats.hpp"
#include "objects.hpp"
#include "actors.hpp"

namespace MWWorld
{
    class CellStore;
}

namespace MWMechanics
{
    class MechanicsManager : public MWBase::MechanicsManager
    {
            MWWorld::Ptr mWatched;
            NpcStats mWatchedStats;
            bool mWatchedStatsEmpty;
            bool mUpdatePlayer;
            bool mClassSelected;
            bool mRaceSelected;
            bool mAI;///< is AI active?

            Objects mObjects;
            Actors mActors;

            typedef std::pair<std::string, bool> Owner; // < Owner id, bool isFaction >
            typedef std::map<Owner, int> OwnerMap; // < Owner, number of stolen items with this id from this owner >
            typedef std::map<std::string, OwnerMap> StolenItemsMap;
            StolenItemsMap mStolenItems;

        public:

            void buildPlayer();
            ///< build player according to stored class/race/birthsign information. Will
            /// default to the values of the ESM::NPC object, if no explicit information is given.

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

            virtual void advanceTime (float duration);

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

            virtual void rest(bool sleep);
            ///< If the player is sleeping or waiting, this should be called every hour.
            /// @param sleep is the player sleeping or waiting?

            virtual int getHoursToRest() const;
            ///< Calculate how many hours the player needs to rest in order to be fully healed

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

            /// Check if \a observer is potentially aware of \a ptr. Does not do a line of sight check!
            virtual bool awarenessCheck (const MWWorld::Ptr& ptr, const MWWorld::Ptr& observer);

            /// Makes \a ptr fight \a target. Also shouts a combat taunt.
            virtual void startCombat (const MWWorld::Ptr& ptr, const MWWorld::Ptr& target);

            /**
             * @note victim may be empty
             * @param arg Depends on \a type, e.g. for Theft, the value of the item that was stolen.
             * @param victimAware Is the victim already aware of the crime?
             *                    If this parameter is false, it will be determined by a line-of-sight and awareness check.
             * @return was the crime seen?
             */
            virtual bool commitCrime (const MWWorld::Ptr& ptr, const MWWorld::Ptr& victim,
                                      OffenseType type, int arg=0, bool victimAware=false);
            /// @return false if the attack was considered a "friendly hit" and forgiven
            virtual bool actorAttacked (const MWWorld::Ptr& victim, const MWWorld::Ptr& attacker);

            /// Notify that actor was killed, add a murder bounty if applicable
            /// @note No-op for non-player attackers
            virtual void actorKilled (const MWWorld::Ptr& victim, const MWWorld::Ptr& attacker);

            /// Utility to check if taking this item is illegal and calling commitCrime if so
            /// @param container The container the item is in; may be empty for an item in the world
            virtual void itemTaken (const MWWorld::Ptr& ptr, const MWWorld::Ptr& item, const MWWorld::Ptr& container,
                                    int count);
            /// Utility to check if opening (i.e. unlocking) this object is illegal and calling commitCrime if so
            virtual void objectOpened (const MWWorld::Ptr& ptr, const MWWorld::Ptr& item);
            /// Attempt sleeping in a bed. If this is illegal, call commitCrime.
            /// @return was it illegal, and someone saw you doing it? Also returns fail when enemies are nearby
            virtual bool sleepInBed (const MWWorld::Ptr& ptr, const MWWorld::Ptr& bed);

            virtual void forceStateUpdate(const MWWorld::Ptr &ptr);

            /// Attempt to play an animation group
            /// @return Success or error
            virtual bool playAnimationGroup(const MWWorld::Ptr& ptr, const std::string& groupName, int mode, int number);
            virtual void skipAnimation(const MWWorld::Ptr& ptr);
            virtual bool checkAnimationPlaying(const MWWorld::Ptr& ptr, const std::string &groupName);

            /// Update magic effects for an actor. Usually done automatically once per frame, but if we're currently
            /// paused we may want to do it manually (after equipping permanent enchantment)
            virtual void updateMagicEffects (const MWWorld::Ptr& ptr);

            virtual void getObjectsInRange (const osg::Vec3f& position, float radius, std::vector<MWWorld::Ptr>& objects);
            virtual void getActorsInRange(const osg::Vec3f &position, float radius, std::vector<MWWorld::Ptr> &objects);

            virtual std::list<MWWorld::Ptr> getActorsFollowing(const MWWorld::Ptr& actor);
            virtual std::list<int> getActorsFollowingIndices(const MWWorld::Ptr& actor);

            virtual std::list<MWWorld::Ptr> getActorsFighting(const MWWorld::Ptr& actor);

            virtual bool toggleAI();
            virtual bool isAIActive();

            virtual void playerLoaded();

            virtual int countSavedGameRecords() const;

            virtual void write (ESM::ESMWriter& writer, Loading::Listener& listener) const;

            virtual void readRecord (ESM::ESMReader& reader, uint32_t type);

            virtual void clear();

            virtual bool isAggressive (const MWWorld::Ptr& ptr, const MWWorld::Ptr& target);

            virtual void keepPlayerAlive();

            virtual bool isReadyToBlock (const MWWorld::Ptr& ptr) const;

            virtual void confiscateStolenItems (const MWWorld::Ptr& player, const MWWorld::Ptr& targetContainer);

            /// List the owners that the player has stolen this item from (the owner can be an NPC or a faction).
            /// <Owner, item count>
            virtual std::vector<std::pair<std::string, int> > getStolenItemOwners(const std::string& itemid);

            /// Has the player stolen this item from the given owner?
            virtual bool isItemStolenFrom(const std::string& itemid, const std::string& ownerid);
            
            /// @return is \a ptr allowed to take/use \a cellref or is it a crime?
            virtual bool isAllowedToUse (const MWWorld::Ptr& ptr, const MWWorld::CellRef& cellref, MWWorld::Ptr& victim);

        private:
            void reportCrime (const MWWorld::Ptr& ptr, const MWWorld::Ptr& victim,
                                      OffenseType type, int arg=0);


    };
}

#endif
