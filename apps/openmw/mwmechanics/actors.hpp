#ifndef GAME_MWMECHANICS_ACTORS_H
#define GAME_MWMECHANICS_ACTORS_H

#include <set>
#include <vector>
#include <string>
#include <list>
#include <map>

namespace ESM
{
    class ESMReader;
    class ESMWriter;
}

namespace osg
{
    class Vec3f;
}

namespace Loading
{
    class Listener;
}

namespace MWWorld
{
    class Ptr;
    class CellStore;
}

namespace MWMechanics
{
    class Actor;
    class CharacterController;
    class CreatureStats;

    class Actors
    {
            std::map<std::string, int> mDeathCount;

            void addBoundItem (const std::string& itemId, const MWWorld::Ptr& actor);
            void removeBoundItem (const std::string& itemId, const MWWorld::Ptr& actor);

            void adjustMagicEffects (const MWWorld::Ptr& creature);

            void calculateDynamicStats (const MWWorld::Ptr& ptr);

            void calculateCreatureStatModifiers (const MWWorld::Ptr& ptr, float duration);
            void calculateNpcStatModifiers (const MWWorld::Ptr& ptr, float duration);

            void calculateRestoration (const MWWorld::Ptr& ptr, float duration);

            void updateDrowning (const MWWorld::Ptr& ptr, float duration, bool isKnockedOut, bool isPlayer);

            void updateEquippedLight (const MWWorld::Ptr& ptr, float duration, bool mayEquip);

            void updateCrimePursuit (const MWWorld::Ptr& ptr, float duration);

            void killDeadActors ();

            void purgeSpellEffects (int casterActorId);

        public:

            Actors();
            ~Actors();

            typedef std::map<MWWorld::Ptr,Actor*> PtrActorMap;

            PtrActorMap::const_iterator begin() { return mActors.begin(); }
            PtrActorMap::const_iterator end() { return mActors.end(); }

            void notifyDied(const MWWorld::Ptr &actor);

            /// Check if the target actor was detected by an observer
            /// If the observer is a non-NPC, check all actors in AI processing distance as observers
            bool isActorDetected(const MWWorld::Ptr& actor, const MWWorld::Ptr& observer);

            /// Update magic effects for an actor. Usually done automatically once per frame, but if we're currently
            /// paused we may want to do it manually (after equipping permanent enchantment)
            void updateMagicEffects (const MWWorld::Ptr& ptr);

            void updateProcessingRange();
            float getProcessingRange() const;

            void addActor (const MWWorld::Ptr& ptr, bool updateImmediately=false);
            ///< Register an actor for stats management
            ///
            /// \note Dead actors are ignored.

            void removeActor (const MWWorld::Ptr& ptr);
            ///< Deregister an actor for stats management
            ///
            /// \note Ignored, if \a ptr is not a registered actor.

            void resurrect (const MWWorld::Ptr& ptr);

            void castSpell(const MWWorld::Ptr& ptr, const std::string spellId, bool manualSpell=false);

            void updateActor(const MWWorld::Ptr &old, const MWWorld::Ptr& ptr);
            ///< Updates an actor with a new Ptr

            void dropActors (const MWWorld::CellStore *cellStore, const MWWorld::Ptr& ignore);
            ///< Deregister all actors (except for \a ignore) in the given cell.

            void updateCombatMusic();
            ///< Update combat music state

            void update (float duration, bool paused);
            ///< Update actor stats and store desired velocity vectors in \a movement

            void updateActor (const MWWorld::Ptr& ptr, float duration);
            ///< This function is normally called automatically during the update process, but it can
            /// also be called explicitly at any time to force an update.

            /** Start combat between two actors
                @Notes: If againstPlayer = true then actor2 should be the Player.
                        If one of the combatants is creature it should be actor1.
            */
            void engageCombat(const MWWorld::Ptr& actor1, const MWWorld::Ptr& actor2, std::map<const MWWorld::Ptr, const std::set<MWWorld::Ptr> >& cachedAllies, bool againstPlayer);

            void playIdleDialogue(const MWWorld::Ptr& actor);
            void updateMovementSpeed(const MWWorld::Ptr& actor);
            void updateGreetingState(const MWWorld::Ptr& actor, bool turnOnly);
            void turnActorToFacePlayer(const MWWorld::Ptr& actor, const osg::Vec3f& dir);

            void updateHeadTracking(const MWWorld::Ptr& actor, const MWWorld::Ptr& targetActor,
                                            MWWorld::Ptr& headTrackTarget, float& sqrHeadTrackDistance);

            void rest(double hours, bool sleep);
            ///< Update actors while the player is waiting or sleeping.

            void updateSneaking(CharacterController* ctrl, float duration);
            ///< Update the sneaking indicator state according to the given player character controller.

            void restoreDynamicStats(const MWWorld::Ptr& actor, double hours, bool sleep);

            int getHoursToRest(const MWWorld::Ptr& ptr) const;
            ///< Calculate how many hours the given actor needs to rest in order to be fully healed

            void fastForwardAi();
            ///< Simulate the passing of time

            int countDeaths (const std::string& id) const;
            ///< Return the number of deaths for actors with the given ID.

            bool isAttackPreparing(const MWWorld::Ptr& ptr);
            bool isRunning(const MWWorld::Ptr& ptr);
            bool isSneaking(const MWWorld::Ptr& ptr);

            void forceStateUpdate(const MWWorld::Ptr &ptr);

            bool playAnimationGroup(const MWWorld::Ptr& ptr, const std::string& groupName, int mode, int number, bool persist=false);
            void skipAnimation(const MWWorld::Ptr& ptr);
            bool checkAnimationPlaying(const MWWorld::Ptr& ptr, const std::string& groupName);
            void persistAnimationStates();

            void getObjectsInRange(const osg::Vec3f& position, float radius, std::vector<MWWorld::Ptr>& out);

            bool isAnyObjectInRange(const osg::Vec3f& position, float radius);

            void cleanupSummonedCreature (CreatureStats& casterStats, int creatureActorId);

            ///Returns the list of actors which are siding with the given actor in fights
            /**ie AiFollow or AiEscort is active and the target is the actor **/
            std::list<MWWorld::Ptr> getActorsSidingWith(const MWWorld::Ptr& actor);
            std::list<MWWorld::Ptr> getActorsFollowing(const MWWorld::Ptr& actor);

            /// Recursive version of getActorsFollowing
            void getActorsFollowing(const MWWorld::Ptr &actor, std::set<MWWorld::Ptr>& out);
            /// Recursive version of getActorsSidingWith
            void getActorsSidingWith(const MWWorld::Ptr &actor, std::set<MWWorld::Ptr>& out);
            /// Recursive version of getActorsSidingWith that takes, adds to and returns a cache of actors mapped to their allies
            void getActorsSidingWith(const MWWorld::Ptr &actor, std::set<MWWorld::Ptr>& out, std::map<const MWWorld::Ptr, const std::set<MWWorld::Ptr> >& cachedAllies);

            /// Get the list of AiFollow::mFollowIndex for all actors following this target
            std::list<int> getActorsFollowingIndices(const MWWorld::Ptr& actor);

            ///Returns the list of actors which are fighting the given actor
            /**ie AiCombat is active and the target is the actor **/
            std::list<MWWorld::Ptr> getActorsFighting(const MWWorld::Ptr& actor);

            /// Unlike getActorsFighting, also returns actors that *would* fight the given actor if they saw him.
            std::list<MWWorld::Ptr> getEnemiesNearby(const MWWorld::Ptr& actor);

            void write (ESM::ESMWriter& writer, Loading::Listener& listener) const;

            void readRecord (ESM::ESMReader& reader, uint32_t type);

            void clear(); // Clear death counter

            bool isCastingSpell(const MWWorld::Ptr& ptr) const;
            bool isReadyToBlock(const MWWorld::Ptr& ptr) const;
            bool isAttackingOrSpell(const MWWorld::Ptr& ptr) const;

    private:
        void updateVisibility (const MWWorld::Ptr& ptr, CharacterController* ctrl);

        PtrActorMap mActors;
        float mTimerDisposeSummonsCorpses;
        float mActorsProcessingRange;

    };
}

#endif
