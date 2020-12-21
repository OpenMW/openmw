#ifndef OPENMW_MWPHYSICS_MTPHYSICS_H
#define OPENMW_MWPHYSICS_MTPHYSICS_H

#include <atomic>
#include <condition_variable>
#include <optional>
#include <shared_mutex>
#include <thread>

#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>

#include <osg/Timer>

#include "physicssystem.hpp"
#include "ptrholder.hpp"

namespace Misc
{
    class Barrier;
}

namespace MWPhysics
{
    class PhysicsTaskScheduler
    {
        public:
            PhysicsTaskScheduler(float physicsDt, std::shared_ptr<btCollisionWorld> collisionWorld);
            ~PhysicsTaskScheduler();

            /// @brief move actors taking into account desired movements and collisions
            /// @param numSteps how much simulation step to run
            /// @param timeAccum accumulated time from previous run to interpolate movements
            /// @param actorsData per actor data needed to compute new positions
            /// @return new position of each actor
            const std::vector<MWWorld::Ptr>& moveActors(int numSteps, float timeAccum, std::vector<ActorFrameData>&& actorsData, osg::Timer_t frameStart, unsigned int frameNumber, osg::Stats& stats);

            const std::vector<MWWorld::Ptr>& resetSimulation(const ActorMap& actors);

            // Thread safe wrappers
            void rayTest(const btVector3& rayFromWorld, const btVector3& rayToWorld, btCollisionWorld::RayResultCallback& resultCallback) const;
            void convexSweepTest(const btConvexShape* castShape, const btTransform& from, const btTransform& to, btCollisionWorld::ConvexResultCallback& resultCallback) const;
            void contactTest(btCollisionObject* colObj, btCollisionWorld::ContactResultCallback& resultCallback);
            std::optional<btVector3> getHitPoint(const btTransform& from, btCollisionObject* target);
            void aabbTest(const btVector3& aabbMin, const btVector3& aabbMax, btBroadphaseAabbCallback& callback);
            void getAabb(const btCollisionObject* obj, btVector3& min, btVector3& max);
            void setCollisionFilterMask(btCollisionObject* collisionObject, int collisionFilterMask);
            void addCollisionObject(btCollisionObject* collisionObject, int collisionFilterGroup, int collisionFilterMask);
            void removeCollisionObject(btCollisionObject* collisionObject);
            void updateSingleAabb(std::weak_ptr<PtrHolder> ptr, bool immediate=false);
            bool getLineOfSight(const std::weak_ptr<Actor>& actor1, const std::weak_ptr<Actor>& actor2);

        private:
            void syncComputation();
            void worker();
            void updateActorsPositions();
            bool hasLineOfSight(const Actor* actor1, const Actor* actor2);
            void refreshLOSCache();
            void updateAabbs();
            void updatePtrAabb(const std::weak_ptr<PtrHolder>& ptr);
            void updateStats(osg::Timer_t frameStart, unsigned int frameNumber, osg::Stats& stats);

            std::unique_ptr<WorldFrameData> mWorldFrameData;
            std::vector<ActorFrameData> mActorsFrameData;
            std::vector<MWWorld::Ptr> mMovedActors;
            const float mPhysicsDt;
            float mTimeAccum;
            std::shared_ptr<btCollisionWorld> mCollisionWorld;
            std::vector<LOSRequest> mLOSCache;
            std::set<std::weak_ptr<PtrHolder>, std::owner_less<std::weak_ptr<PtrHolder>>> mUpdateAabb;

            // TODO: use std::experimental::flex_barrier or std::barrier once it becomes a thing
            std::unique_ptr<Misc::Barrier> mPreStepBarrier;
            std::unique_ptr<Misc::Barrier> mPostStepBarrier;
            std::unique_ptr<Misc::Barrier> mPostSimBarrier;

            int mNumThreads;
            int mNumJobs;
            int mRemainingSteps;
            int mLOSCacheExpiry;
            bool mDeferAabbUpdate;
            bool mNewFrame;
            bool mAdvanceSimulation;
            bool mThreadSafeBullet;
            bool mQuit;
            std::atomic<int> mNextJob;
            std::atomic<int> mNextLOS;
            std::vector<std::thread> mThreads;

            mutable std::shared_mutex mSimulationMutex;
            mutable std::shared_mutex mCollisionWorldMutex;
            mutable std::shared_mutex mLOSCacheMutex;
            mutable std::mutex mUpdateAabbMutex;
            std::condition_variable_any mHasJob;

            unsigned int mFrameNumber;
            const osg::Timer* mTimer;
            osg::Timer_t mTimeBegin;
            osg::Timer_t mTimeEnd;
            osg::Timer_t mFrameStart;
    };

}
#endif
