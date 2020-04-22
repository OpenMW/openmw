#ifndef GAME_MWMECHANICS_MECHANICSMANAGERIMP_H
#define GAME_MWMECHANICS_MECHANICSMANAGERIMP_H

#include <components/settings/settings.hpp>

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

            AttributeValue mWatchedAttributes[8];
            SkillValue mWatchedSkills[27];

            DynamicStat<float> mWatchedHealth;
            DynamicStat<float> mWatchedMagicka;
            DynamicStat<float> mWatchedFatigue;

            int mWatchedLevel;

            float mWatchedTimeToStartDrowning;

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

            virtual void add (const MWWorld::Ptr& ptr) override;
            ///< Register an object for management

            virtual void remove (const MWWorld::Ptr& ptr) override;
            ///< Deregister an object for management

            virtual void updateCell(const MWWorld::Ptr &old, const MWWorld::Ptr &ptr) override;
            ///< Moves an object to a new cell

            virtual void drop(const MWWorld::CellStore *cellStore) override;
            ///< Deregister all objects in the given cell.

            virtual void watchActor(const MWWorld::Ptr& ptr) override;
            ///< On each update look for changes in a previously registered actor and update the
            /// GUI accordingly.

            virtual void update (float duration, bool paused) override;
            ///< Update objects
            ///
            /// \param paused In game type does not currently advance (this usually means some GUI
            /// component is up).

            virtual void setPlayerName (const std::string& name) override;
            ///< Set player name.

            virtual void setPlayerRace (const std::string& id, bool male, const std::string &head, const std::string &hair) override;
            ///< Set player race.

            virtual void setPlayerBirthsign (const std::string& id) override;
            ///< Set player birthsign.

            virtual void setPlayerClass (const std::string& id) override;
            ///< Set player class to stock class.

            virtual void setPlayerClass (const ESM::Class& class_) override;
            ///< Set player class to custom class.

            virtual void restoreDynamicStats(MWWorld::Ptr actor, double hours, bool sleep) override;

            virtual void rest(double hours, bool sleep) override;
            ///< If the player is sleeping or waiting, this should be called every hour.
            /// @param sleep is the player sleeping or waiting?

            virtual int getHoursToRest() const override;
            ///< Calculate how many hours the player needs to rest in order to be fully healed

            virtual int getBarterOffer(const MWWorld::Ptr& ptr,int basePrice, bool buying) override;
            ///< This is used by every service to determine the price of objects given the trading skills of the player and NPC.

            virtual int getDerivedDisposition(const MWWorld::Ptr& ptr, bool addTemporaryDispositionChange = true) override;
            ///< Calculate the diposition of an NPC toward the player.

            virtual int countDeaths (const std::string& id) const override;
            ///< Return the number of deaths for actors with the given ID.

            virtual void getPersuasionDispositionChange (const MWWorld::Ptr& npc, PersuasionType type, bool& success, float& tempChange, float& permChange) override;
            ///< Perform a persuasion action on NPC

            /// Check if \a observer is potentially aware of \a ptr. Does not do a line of sight check!
            virtual bool awarenessCheck (const MWWorld::Ptr& ptr, const MWWorld::Ptr& observer) override;

            /// Makes \a ptr fight \a target. Also shouts a combat taunt.
            virtual void startCombat (const MWWorld::Ptr& ptr, const MWWorld::Ptr& target) override;

            /**
             * @note victim may be empty
             * @param arg Depends on \a type, e.g. for Theft, the value of the item that was stolen.
             * @param victimAware Is the victim already aware of the crime?
             *                    If this parameter is false, it will be determined by a line-of-sight and awareness check.
             * @return was the crime seen?
             */
            virtual bool commitCrime (const MWWorld::Ptr& ptr, const MWWorld::Ptr& victim,
                                      OffenseType type, const std::string& factionId="", int arg=0, bool victimAware=false) override;
            /// @return false if the attack was considered a "friendly hit" and forgiven
            virtual bool actorAttacked (const MWWorld::Ptr& victim, const MWWorld::Ptr& attacker) override;

            /// Notify that actor was killed, add a murder bounty if applicable
            /// @note No-op for non-player attackers
            virtual void actorKilled (const MWWorld::Ptr& victim, const MWWorld::Ptr& attacker) override;

            /// Utility to check if taking this item is illegal and calling commitCrime if so
            /// @param container The container the item is in; may be empty for an item in the world
            virtual void itemTaken (const MWWorld::Ptr& ptr, const MWWorld::Ptr& item, const MWWorld::Ptr& container,
                                    int count, bool alarm = true) override;
            /// Utility to check if unlocking this object is illegal and calling commitCrime if so
            virtual void unlockAttempted (const MWWorld::Ptr& ptr, const MWWorld::Ptr& item) override;
            /// Attempt sleeping in a bed. If this is illegal, call commitCrime.
            /// @return was it illegal, and someone saw you doing it? Also returns fail when enemies are nearby
            virtual bool sleepInBed (const MWWorld::Ptr& ptr, const MWWorld::Ptr& bed) override;

            virtual void forceStateUpdate(const MWWorld::Ptr &ptr) override;

            /// Attempt to play an animation group
            /// @return Success or error
            virtual bool playAnimationGroup(const MWWorld::Ptr& ptr, const std::string& groupName, int mode, int number, bool persist=false) override;
            virtual void skipAnimation(const MWWorld::Ptr& ptr) override;
            virtual bool checkAnimationPlaying(const MWWorld::Ptr& ptr, const std::string &groupName) override;
            virtual void persistAnimationStates() override;

            /// Update magic effects for an actor. Usually done automatically once per frame, but if we're currently
            /// paused we may want to do it manually (after equipping permanent enchantment)
            virtual void updateMagicEffects (const MWWorld::Ptr& ptr) override;

            virtual void getObjectsInRange (const osg::Vec3f& position, float radius, std::vector<MWWorld::Ptr>& objects) override;
            virtual void getActorsInRange(const osg::Vec3f &position, float radius, std::vector<MWWorld::Ptr> &objects) override;

            /// Check if there are actors in selected range
            virtual bool isAnyActorInRange(const osg::Vec3f &position, float radius) override;

            virtual std::list<MWWorld::Ptr> getActorsSidingWith(const MWWorld::Ptr& actor) override;
            virtual std::list<MWWorld::Ptr> getActorsFollowing(const MWWorld::Ptr& actor) override;
            virtual std::list<int> getActorsFollowingIndices(const MWWorld::Ptr& actor) override;

            virtual std::list<MWWorld::Ptr> getActorsFighting(const MWWorld::Ptr& actor) override;
            virtual std::list<MWWorld::Ptr> getEnemiesNearby(const MWWorld::Ptr& actor) override;

            /// Recursive version of getActorsFollowing
            virtual void getActorsFollowing(const MWWorld::Ptr& actor, std::set<MWWorld::Ptr>& out) override;
            /// Recursive version of getActorsSidingWith
            virtual void getActorsSidingWith(const MWWorld::Ptr& actor, std::set<MWWorld::Ptr>& out) override;

            virtual bool toggleAI() override;
            virtual bool isAIActive() override;

            virtual void playerLoaded() override;

            virtual bool onOpen(const MWWorld::Ptr& ptr) override;
            virtual void onClose(const MWWorld::Ptr& ptr) override;

            virtual int countSavedGameRecords() const override;

            virtual void write (ESM::ESMWriter& writer, Loading::Listener& listener) const override;

            virtual void readRecord (ESM::ESMReader& reader, uint32_t type) override;

            virtual void clear() override;

            virtual bool isAggressive (const MWWorld::Ptr& ptr, const MWWorld::Ptr& target) override;

            virtual void resurrect(const MWWorld::Ptr& ptr) override;

            virtual bool isCastingSpell (const MWWorld::Ptr& ptr) const override;

            virtual bool isReadyToBlock (const MWWorld::Ptr& ptr) const override;
            /// Is \a ptr casting spell or using weapon now?
            virtual bool isAttackingOrSpell(const MWWorld::Ptr &ptr) const override;

            virtual void castSpell(const MWWorld::Ptr& ptr, const std::string spellId, bool manualSpell=false) override;

            void processChangedSettings(const Settings::CategorySettingVector& settings) override;

            virtual float getActorsProcessingRange() const override;

            virtual void notifyDied(const MWWorld::Ptr& actor) override;

            /// Check if the target actor was detected by an observer
            /// If the observer is a non-NPC, check all actors in AI processing distance as observers
            virtual bool isActorDetected(const MWWorld::Ptr& actor, const MWWorld::Ptr& observer) override;

            virtual void confiscateStolenItems (const MWWorld::Ptr& player, const MWWorld::Ptr& targetContainer) override;

            /// List the owners that the player has stolen this item from (the owner can be an NPC or a faction).
            /// <Owner, item count>
            virtual std::vector<std::pair<std::string, int> > getStolenItemOwners(const std::string& itemid) override;

            /// Has the player stolen this item from the given owner?
            virtual bool isItemStolenFrom(const std::string& itemid, const MWWorld::Ptr& ptr) override;

            virtual bool isBoundItem(const MWWorld::Ptr& item) override;

            /// @return is \a ptr allowed to take/use \a target or is it a crime?
            virtual bool isAllowedToUse (const MWWorld::Ptr& ptr, const MWWorld::Ptr& target, MWWorld::Ptr& victim) override;

            virtual void setWerewolf(const MWWorld::Ptr& actor, bool werewolf) override;
            virtual void applyWerewolfAcrobatics(const MWWorld::Ptr& actor) override;

            virtual void cleanupSummonedCreature(const MWWorld::Ptr& caster, int creatureActorId) override;

            virtual void confiscateStolenItemToOwner(const MWWorld::Ptr &player, const MWWorld::Ptr &item, const MWWorld::Ptr& victim, int count) override;

            virtual bool isAttackPreparing(const MWWorld::Ptr& ptr) override;
            virtual bool isRunning(const MWWorld::Ptr& ptr) override;
            virtual bool isSneaking(const MWWorld::Ptr& ptr) override;

        private:
            bool canCommitCrimeAgainst(const MWWorld::Ptr& victim, const MWWorld::Ptr& attacker);
            bool canReportCrime(const MWWorld::Ptr &actor, const MWWorld::Ptr &victim, std::set<MWWorld::Ptr> &playerFollowers);

            bool reportCrime (const MWWorld::Ptr& ptr, const MWWorld::Ptr& victim,
                                      OffenseType type, const std::string& factionId, int arg=0);
    };
}

#endif
