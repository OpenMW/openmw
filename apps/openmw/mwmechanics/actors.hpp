#ifndef GAME_MWMECHANICS_ACTORS_H
#define GAME_MWMECHANICS_ACTORS_H

#include <set>
#include <vector>
#include <string>
#include <map>
#include <list>

#include "../mwbase/world.hpp"

#include "movement.hpp"

namespace MWWorld
{
    class Ptr;
    class CellStore;
}

namespace MWMechanics
{
    class Actor;
    class CreatureStats;

    class Actors
    {
            std::map<std::string, int> mDeathCount;

            void updateNpc(const MWWorld::Ptr &ptr, float duration);

            void adjustMagicEffects (const MWWorld::Ptr& creature);

            void calculateDynamicStats (const MWWorld::Ptr& ptr);

            void calculateCreatureStatModifiers (const MWWorld::Ptr& ptr, float duration);
            void calculateNpcStatModifiers (const MWWorld::Ptr& ptr, float duration);

            void calculateRestoration (const MWWorld::Ptr& ptr, float duration);

            void updateDrowning (const MWWorld::Ptr& ptr, float duration);

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

            /// Check if the target actor was detected by an observer
            /// If the observer is a non-NPC, check all actors in AI processing distance as observers
            bool isActorDetected(const MWWorld::Ptr& actor, const MWWorld::Ptr& observer);

            /// Update magic effects for an actor. Usually done automatically once per frame, but if we're currently
            /// paused we may want to do it manually (after equipping permanent enchantment)
            void updateMagicEffects (const MWWorld::Ptr& ptr);

            void addActor (const MWWorld::Ptr& ptr, bool updateImmediately=false);
            ///< Register an actor for stats management
            ///
            /// \note Dead actors are ignored.

            void removeActor (const MWWorld::Ptr& ptr);
            ///< Deregister an actor for stats management
            ///
            /// \note Ignored, if \a ptr is not a registered actor.

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

            void updateHeadTracking(const MWWorld::Ptr& actor, const MWWorld::Ptr& targetActor,
                                            MWWorld::Ptr& headTrackTarget, float& sqrHeadTrackDistance);

            void rest(bool sleep);
            ///< Update actors while the player is waiting or sleeping. This should be called every hour.

            void restoreDynamicStats(const MWWorld::Ptr& actor, bool sleep);

            int getHoursToRest(const MWWorld::Ptr& ptr) const;
            ///< Calculate how many hours the given actor needs to rest in order to be fully healed

            void fastForwardAi();
            ///< Simulate the passing of time

            int countDeaths (const std::string& id) const;
            ///< Return the number of deaths for actors with the given ID.

            bool isAttackPrepairing(const MWWorld::Ptr& ptr);
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

            bool isReadyToBlock(const MWWorld::Ptr& ptr) const;
            bool isAttackingOrSpell(const MWWorld::Ptr& ptr) const;

    private:
        PtrActorMap mActors;
        float mTimerDisposeSummonsCorpses;

    };
}

#endif
