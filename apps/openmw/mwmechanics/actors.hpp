#ifndef GAME_MWMECHANICS_ACTORS_H
#define GAME_MWMECHANICS_ACTORS_H

#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "actor.hpp"

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
    class SidingCache;

    class Actors
    {
    public:
        std::list<Actor>::const_iterator begin() const { return mActors.begin(); }
        std::list<Actor>::const_iterator end() const { return mActors.end(); }
        std::size_t size() const { return mActors.size(); }

        void notifyDied(const MWWorld::Ptr& actor);

        /// Check if the target actor was detected by an observer
        /// If the observer is a non-NPC, check all actors in AI processing distance as observers
        bool isActorDetected(const MWWorld::Ptr& actor, const MWWorld::Ptr& observer) const;

        /// Update magic effects for an actor. Usually done automatically once per frame, but if we're currently
        /// paused we may want to do it manually (after equipping permanent enchantment)
        void updateMagicEffects(const MWWorld::Ptr& ptr) const;

        void addActor(const MWWorld::Ptr& ptr, bool updateImmediately = false);
        ///< Register an actor for stats management
        ///
        /// \note Dead actors are ignored.

        void removeActor(const MWWorld::Ptr& ptr, bool keepActive);
        ///< Deregister an actor for stats management
        ///
        /// \note Ignored, if \a ptr is not a registered actor.

        void resurrect(const MWWorld::Ptr& ptr) const;

        void castSpell(const MWWorld::Ptr& ptr, const ESM::RefId& spellId, bool scriptedSpell = false) const;

        void updateActor(const MWWorld::Ptr& old, const MWWorld::Ptr& ptr) const;
        ///< Updates an actor with a new Ptr

        void dropActors(const MWWorld::CellStore* cellStore, const MWWorld::Ptr& ignore);
        ///< Deregister all actors (except for \a ignore) in the given cell.

        void update(float duration, bool paused);
        ///< Update actor stats and store desired velocity vectors in \a movement

        void updateActor(const MWWorld::Ptr& ptr, float duration) const;
        ///< This function is normally called automatically during the update process, but it can
        /// also be called explicitly at any time to force an update.

        /// Removes an actor from combat and makes all of their allies stop fighting the actor's targets
        void stopCombat(const MWWorld::Ptr& ptr) const;

        void playIdleDialogue(const MWWorld::Ptr& actor) const;
        void updateMovementSpeed(const MWWorld::Ptr& actor) const;
        void updateGreetingState(const MWWorld::Ptr& actor, Actor& actorState, bool turnOnly);
        void turnActorToFacePlayer(const MWWorld::Ptr& actor, Actor& actorState, const osg::Vec3f& dir) const;

        void rest(double hours, bool sleep) const;
        ///< Update actors while the player is waiting or sleeping.

        void updateSneaking(CharacterController* ctrl, float duration);
        ///< Update the sneaking indicator state according to the given player character controller.

        void restoreDynamicStats(const MWWorld::Ptr& actor, double hours, bool sleep) const;

        int getHoursToRest(const MWWorld::Ptr& ptr) const;
        ///< Calculate how many hours the given actor needs to rest in order to be fully healed

        void fastForwardAi() const;
        ///< Simulate the passing of time

        int countDeaths(const ESM::RefId& id) const;
        ///< Return the number of deaths for actors with the given ID.

        bool isAttackPreparing(const MWWorld::Ptr& ptr) const;
        bool isRunning(const MWWorld::Ptr& ptr) const;
        bool isSneaking(const MWWorld::Ptr& ptr) const;

        void forceStateUpdate(const MWWorld::Ptr& ptr) const;

        bool playAnimationGroup(const MWWorld::Ptr& ptr, std::string_view groupName, int mode, uint32_t number,
            bool scripted = false) const;
        bool playAnimationGroupLua(const MWWorld::Ptr& ptr, std::string_view groupName, uint32_t loops, float speed,
            std::string_view startKey, std::string_view stopKey, bool forceLoop);
        void enableLuaAnimations(const MWWorld::Ptr& ptr, bool enable);
        void skipAnimation(const MWWorld::Ptr& ptr) const;
        bool checkAnimationPlaying(const MWWorld::Ptr& ptr, const std::string& groupName) const;
        bool checkScriptedAnimationPlaying(const MWWorld::Ptr& ptr) const;
        void persistAnimationStates() const;
        void clearAnimationQueue(const MWWorld::Ptr& ptr, bool clearScripted);

        void getObjectsInRange(const osg::Vec3f& position, float radius, std::vector<MWWorld::Ptr>& out) const;

        bool isAnyObjectInRange(const osg::Vec3f& position, float radius) const;

        void cleanupSummonedCreature(CreatureStats& casterStats, int creatureActorId) const;

        /// Returns the list of actors which are siding with the given actor in fights
        /**ie AiFollow or AiEscort is active and the target is the actor **/
        std::vector<MWWorld::Ptr> getActorsSidingWith(const MWWorld::Ptr& actor, bool excludeInfighting = false) const;
        std::vector<MWWorld::Ptr> getActorsFollowing(const MWWorld::Ptr& actor) const;

        /// Recursive version of getActorsFollowing
        void getActorsFollowing(const MWWorld::Ptr& actor, std::set<MWWorld::Ptr>& out) const;
        /// Recursive version of getActorsSidingWith
        void getActorsSidingWith(
            const MWWorld::Ptr& actor, std::set<MWWorld::Ptr>& out, bool excludeInfighting = false) const;

        /// Get the list of AiFollow::mFollowIndex for all actors following this target
        std::vector<int> getActorsFollowingIndices(const MWWorld::Ptr& actor) const;
        std::map<int, MWWorld::Ptr> getActorsFollowingByIndex(const MWWorld::Ptr& actor) const;

        /// Returns the list of actors which are fighting the given actor
        /**ie AiCombat is active and the target is the actor **/
        std::vector<MWWorld::Ptr> getActorsFighting(const MWWorld::Ptr& actor) const;

        /// Unlike getActorsFighting, also returns actors that *would* fight the given actor if they saw him.
        std::vector<MWWorld::Ptr> getEnemiesNearby(const MWWorld::Ptr& actor) const;

        void write(ESM::ESMWriter& writer, Loading::Listener& listener) const;

        void readRecord(ESM::ESMReader& reader, uint32_t type);

        void clear(); // Clear death counter

        bool isCastingSpell(const MWWorld::Ptr& ptr) const;
        bool isReadyToBlock(const MWWorld::Ptr& ptr) const;
        bool isAttackingOrSpell(const MWWorld::Ptr& ptr) const;

        int getGreetingTimer(const MWWorld::Ptr& ptr) const;
        float getAngleToPlayer(const MWWorld::Ptr& ptr) const;
        GreetingState getGreetingState(const MWWorld::Ptr& ptr) const;
        bool isTurningToPlayer(const MWWorld::Ptr& ptr) const;

    private:
        std::map<ESM::RefId, int> mDeathCount;
        std::list<Actor> mActors;
        std::map<const MWWorld::LiveCellRefBase*, std::list<Actor>::iterator> mIndex;
        // We should add a delay between summoned creature death and its corpse despawning
        float mTimerDisposeSummonsCorpses = 0.2f;
        float mTimerUpdateHeadTrack = 0;
        float mTimerUpdateEquippedLight = 0;
        float mTimerUpdateHello = 0;
        float mSneakTimer = 0; // Times update of sneak icon
        float mSneakSkillTimer = 0; // Times sneak skill progress from "avoid notice"

        void updateVisibility(const MWWorld::Ptr& ptr, CharacterController& ctrl) const;

        void adjustMagicEffects(const MWWorld::Ptr& creature, float duration) const;

        void calculateRestoration(const MWWorld::Ptr& ptr, float duration) const;

        void updateCrimePursuit(const MWWorld::Ptr& ptr, float duration, SidingCache& cachedAllies) const;

        void killDeadActors();

        void purgeSpellEffects(int casterActorId) const;

        void predictAndAvoidCollisions(float duration) const;

        /** Start combat between two actors
            @Notes: If againstPlayer = true then actor2 should be the Player.
                    If one of the combatants is creature it should be actor1.
        */
        void engageCombat(const MWWorld::Ptr& actor1, const MWWorld::Ptr& actor2, SidingCache& cachedAllies,
            bool againstPlayer) const;
    };

    class SidingCache
    {
        const Actors& mActors;
        const bool mExcludeInfighting;
        std::map<MWWorld::Ptr, std::set<MWWorld::Ptr>> mCache;

    public:
        SidingCache(const Actors& actors, bool excludeInfighting)
            : mActors(actors)
            , mExcludeInfighting(excludeInfighting)
        {
        }

        /// Recursive version of getActorsSidingWith that takes, returns a cached set of allies
        const std::set<MWWorld::Ptr>& getActorsSidingWith(const MWWorld::Ptr& actor);
    };
}

#endif
