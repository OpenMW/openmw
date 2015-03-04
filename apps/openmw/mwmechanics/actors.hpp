#ifndef GAME_MWMECHANICS_ACTORS_H
#define GAME_MWMECHANICS_ACTORS_H

#include <set>
#include <vector>
#include <string>
#include <map>
#include <list>

#include "movement.hpp"
#include "../mwbase/world.hpp"

namespace Ogre
{
    class Vector3;
}

namespace MWWorld
{
    class Ptr;
    class CellStore;
}

namespace MWMechanics
{
    class Actor;

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

            void updateEquippedLight (const MWWorld::Ptr& ptr, float duration);

            void updateCrimePersuit (const MWWorld::Ptr& ptr, float duration);

            void killDeadActors ();

        public:

            Actors();
            ~Actors();

            typedef std::map<MWWorld::Ptr,Actor*> PtrActorMap;

            PtrActorMap::const_iterator begin() { return mActors.begin(); }
            PtrActorMap::const_iterator end() { return mActors.end(); }

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

            void update (float duration, bool paused);
            ///< Update actor stats and store desired velocity vectors in \a movement

            void updateActor (const MWWorld::Ptr& ptr, float duration);
            ///< This function is normally called automatically during the update process, but it can
            /// also be called explicitly at any time to force an update.

            /** Start combat between two actors
                @Notes: If againstPlayer = true then actor2 should be the Player.
                        If one of the combatants is creature it should be actor1.
            */
            void engageCombat(const MWWorld::Ptr& actor1, const MWWorld::Ptr& actor2, bool againstPlayer);

            void updateHeadTracking(const MWWorld::Ptr& actor, const MWWorld::Ptr& targetActor,
                                            MWWorld::Ptr& headTrackTarget, float& sqrHeadTrackDistance);

            void restoreDynamicStats(bool sleep);
            ///< If the player is sleeping, this should be called every hour.

            void restoreDynamicStats(const MWWorld::Ptr& actor, bool sleep);

            int getHoursToRest(const MWWorld::Ptr& ptr) const;
            ///< Calculate how many hours the given actor needs to rest in order to be fully healed

            void fastForwardAi();
            ///< Simulate the passing of time

            int countDeaths (const std::string& id) const;
            ///< Return the number of deaths for actors with the given ID.

        void forceStateUpdate(const MWWorld::Ptr &ptr);

        void playAnimationGroup(const MWWorld::Ptr& ptr, const std::string& groupName, int mode, int number);
        void skipAnimation(const MWWorld::Ptr& ptr);
        bool checkAnimationPlaying(const MWWorld::Ptr& ptr, const std::string& groupName);

            void getObjectsInRange(const Ogre::Vector3& position, float radius, std::vector<MWWorld::Ptr>& out);

            ///Returns the list of actors which are following the given actor
            /**ie AiFollow is active and the target is the actor **/
            std::list<MWWorld::Ptr> getActorsFollowing(const MWWorld::Ptr& actor);

            /// Get the list of AiFollow::mFollowIndex for all actors following this target
            std::list<int> getActorsFollowingIndices(const MWWorld::Ptr& actor);

            ///Returns the list of actors which are fighting the given actor
            /**ie AiCombat is active and the target is the actor **/
            std::list<MWWorld::Ptr> getActorsFighting(const MWWorld::Ptr& actor);

            void write (ESM::ESMWriter& writer, Loading::Listener& listener) const;

            void readRecord (ESM::ESMReader& reader, uint32_t type);

            void clear(); // Clear death counter

            bool isReadyToBlock(const MWWorld::Ptr& ptr) const;

    private:
        PtrActorMap mActors;

    };
}

#endif
