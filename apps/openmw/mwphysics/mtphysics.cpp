#include <BulletCollision/BroadphaseCollision/btDbvtBroadphase.h>
#include <BulletCollision/CollisionShapes/btCollisionShape.h>

#include <osg/Stats>

#include "components/debug/debuglog.hpp"
#include <components/misc/barrier.hpp>
#include "components/misc/convert.hpp"
#include "components/settings/settings.hpp"
#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/movement.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/player.hpp"

#include "actor.hpp"
#include "movementsolver.hpp"
#include "mtphysics.hpp"
#include "object.hpp"
#include "physicssystem.hpp"
#include "projectile.hpp"

namespace
{
    /// @brief A scoped lock that is either shared or exclusive depending on configuration
    template<class Mutex>
    class MaybeSharedLock
    {
        public:
            /// @param mutex a shared mutex
            /// @param canBeSharedLock decide wether the lock will be shared or exclusive
            MaybeSharedLock(Mutex& mutex, bool canBeSharedLock) : mMutex(mutex), mCanBeSharedLock(canBeSharedLock)
            {
                if (mCanBeSharedLock)
                    mMutex.lock_shared();
                else
                    mMutex.lock();
            }

            ~MaybeSharedLock()
            {
                if (mCanBeSharedLock)
                    mMutex.unlock_shared();
                else
                    mMutex.unlock();
            }
        private:
            Mutex& mMutex;
            bool mCanBeSharedLock;
    };

    void handleFall(MWPhysics::ActorFrameData& actorData, bool simulationPerformed)
    {
        const float heightDiff = actorData.mPosition.z() - actorData.mOldHeight;

        const bool isStillOnGround = (simulationPerformed && actorData.mWasOnGround && actorData.mActorRaw->getOnGround());

        if (isStillOnGround || actorData.mFlying || actorData.mSwimming || actorData.mSlowFall < 1)
            actorData.mNeedLand = true;
        else if (heightDiff < 0)
            actorData.mFallHeight += heightDiff;
    }

    void handleJump(const MWWorld::Ptr &ptr)
    {
        const bool isPlayer = (ptr == MWMechanics::getPlayer());
        // Advance acrobatics and set flag for GetPCJumping
        if (isPlayer)
        {
            ptr.getClass().skillUsageSucceeded(ptr, ESM::Skill::Acrobatics, 0);
            MWBase::Environment::get().getWorld()->getPlayer().setJumping(true);
        }

        // Decrease fatigue
        if (!isPlayer || !MWBase::Environment::get().getWorld()->getGodModeState())
        {
            const MWWorld::Store<ESM::GameSetting> &gmst = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();
            const float fFatigueJumpBase = gmst.find("fFatigueJumpBase")->mValue.getFloat();
            const float fFatigueJumpMult = gmst.find("fFatigueJumpMult")->mValue.getFloat();
            const float normalizedEncumbrance = std::min(1.f, ptr.getClass().getNormalizedEncumbrance(ptr));
            const float fatigueDecrease = fFatigueJumpBase + normalizedEncumbrance * fFatigueJumpMult;
            MWMechanics::DynamicStat<float> fatigue = ptr.getClass().getCreatureStats(ptr).getFatigue();
            fatigue.setCurrent(fatigue.getCurrent() - fatigueDecrease);
            ptr.getClass().getCreatureStats(ptr).setFatigue(fatigue);
        }
        ptr.getClass().getMovementSettings(ptr).mPosition[2] = 0;
    }

    void updateMechanics(MWPhysics::ActorFrameData& actorData)
    {
        if (actorData.mDidJump)
            handleJump(actorData.mPtr);

        MWMechanics::CreatureStats& stats = actorData.mPtr.getClass().getCreatureStats(actorData.mPtr);
        if (actorData.mNeedLand)
            stats.land(actorData.mPtr == MWMechanics::getPlayer() && (actorData.mFlying || actorData.mSwimming));
        else if (actorData.mFallHeight < 0)
            stats.addToFallHeight(-actorData.mFallHeight);
    }

    osg::Vec3f interpolateMovements(MWPhysics::ActorFrameData& actorData, float timeAccum, float physicsDt)
    {
        const float interpolationFactor = timeAccum / physicsDt;

        // account for force change of actor's position in the main thread
        const auto correction = actorData.mActorRaw->getWorldPosition() - actorData.mOrigin;
        if (correction.length() != 0)
        {
            actorData.mActorRaw->adjustPosition(correction);
            actorData.mPosition = actorData.mActorRaw->getPosition();
        }

        return actorData.mPosition * interpolationFactor + actorData.mActorRaw->getPreviousPosition() * (1.f - interpolationFactor);
    }

    struct WorldFrameData
    {
        WorldFrameData() : mIsInStorm(MWBase::Environment::get().getWorld()->isInStorm())
                         , mStormDirection(MWBase::Environment::get().getWorld()->getStormDirection())
        {}

        bool mIsInStorm;
        osg::Vec3f mStormDirection;
    };

    namespace Config
    {
        /// @return either the number of thread as configured by the user, or 1 if Bullet doesn't support multithreading
        int computeNumThreads(bool& threadSafeBullet)
        {
            int wantedThread = Settings::Manager::getInt("async num threads", "Physics");

            auto broad = std::make_unique<btDbvtBroadphase>();
            auto maxSupportedThreads = broad->m_rayTestStacks.size();
            threadSafeBullet = (maxSupportedThreads > 1);
            if (!threadSafeBullet && wantedThread > 1)
            {
                Log(Debug::Warning) << "Bullet was not compiled with multithreading support, 1 async thread will be used";
                return 1;
            }
            return std::max(0, wantedThread);
        }
    }
}

namespace MWPhysics
{
    PhysicsTaskScheduler::PhysicsTaskScheduler(float physicsDt, std::shared_ptr<btCollisionWorld> collisionWorld)
          : mPhysicsDt(physicsDt)
          , mTimeAccum(0.f)
          , mCollisionWorld(std::move(collisionWorld))
          , mNumJobs(0)
          , mRemainingSteps(0)
          , mLOSCacheExpiry(Settings::Manager::getInt("lineofsight keep inactive cache", "Physics"))
          , mDeferAabbUpdate(Settings::Manager::getBool("defer aabb update", "Physics"))
          , mNewFrame(false)
          , mAdvanceSimulation(false)
          , mQuit(false)
          , mNextJob(0)
          , mNextLOS(0)
          , mFrameNumber(0)
          , mTimer(osg::Timer::instance())
    {
        mNumThreads = Config::computeNumThreads(mThreadSafeBullet);

        if (mNumThreads >= 1)
        {
            for (int i = 0; i < mNumThreads; ++i)
                mThreads.emplace_back([&] { worker(); } );
        }
        else
        {
            mLOSCacheExpiry = -1;
            mDeferAabbUpdate = false;
        }

        mPreStepBarrier = std::make_unique<Misc::Barrier>(mNumThreads, [&]()
            {
                updateAabbs();
            });

        mPostStepBarrier = std::make_unique<Misc::Barrier>(mNumThreads, [&]()
            {
                if (mRemainingSteps)
                    --mRemainingSteps;
                mNextJob.store(0, std::memory_order_release);
                updateActorsPositions();
            });

        mPostSimBarrier = std::make_unique<Misc::Barrier>(mNumThreads, [&]()
            {
                mNewFrame = false;
                if (mLOSCacheExpiry >= 0)
                {
                    std::unique_lock lock(mLOSCacheMutex);
                    mLOSCache.erase(
                            std::remove_if(mLOSCache.begin(), mLOSCache.end(),
                                [](const LOSRequest& req) { return req.mStale; }),
                            mLOSCache.end());
                }
                mTimeEnd = mTimer->tick();
            });
    }

    PhysicsTaskScheduler::~PhysicsTaskScheduler()
    {
        std::unique_lock lock(mSimulationMutex);
        mQuit = true;
        mNumJobs = 0;
        mRemainingSteps = 0;
        lock.unlock();
        mHasJob.notify_all();
        for (auto& thread : mThreads)
            thread.join();
    }

    const PtrPositionList& PhysicsTaskScheduler::moveActors(int numSteps, float timeAccum, std::vector<ActorFrameData>&& actorsData, osg::Timer_t frameStart, unsigned int frameNumber, osg::Stats& stats)
    {
        // This function run in the main thread.
        // While the mSimulationMutex is held, background physics threads can't run.

        std::unique_lock lock(mSimulationMutex);

        for (auto& data : actorsData)
            data.updatePosition();

        // start by finishing previous background computation
        if (mNumThreads != 0)
        {
            for (auto& data : mActorsFrameData)
            {
                // Ignore actors that were deleted while the background thread was running
                if (!data.mActor.lock())
                    continue;

                updateMechanics(data);
                if (mAdvanceSimulation)
                    data.mActorRaw->setStandingOnPtr(data.mStandingOn);

                if (mMovementResults.find(data.mPtr) != mMovementResults.end())
                    data.mActorRaw->setSimulationPosition(mMovementResults[data.mPtr]);
            }

            if (mFrameNumber == frameNumber - 1)
            {
                stats.setAttribute(mFrameNumber, "physicsworker_time_begin", mTimer->delta_s(mFrameStart, mTimeBegin));
                stats.setAttribute(mFrameNumber, "physicsworker_time_taken", mTimer->delta_s(mTimeBegin, mTimeEnd));
                stats.setAttribute(mFrameNumber, "physicsworker_time_end", mTimer->delta_s(mFrameStart, mTimeEnd));
            }
            mFrameStart = frameStart;
            mTimeBegin = mTimer->tick();
            mFrameNumber = frameNumber;
        }

        // init
        mRemainingSteps = numSteps;
        mTimeAccum = timeAccum;
        mActorsFrameData = std::move(actorsData);
        mAdvanceSimulation = (mRemainingSteps != 0);
        mNewFrame = true;
        mNumJobs = mActorsFrameData.size();
        mNextLOS.store(0, std::memory_order_relaxed);
        mNextJob.store(0, std::memory_order_release);

        if (mAdvanceSimulation)
            mWorldFrameData = std::make_unique<WorldFrameData>();

        if (mNumThreads == 0)
        {
            mMovementResults.clear();
            syncComputation();

            for (auto& data : mActorsFrameData)
            {
                if (mAdvanceSimulation)
                    data.mActorRaw->setStandingOnPtr(data.mStandingOn);
                if (mMovementResults.find(data.mPtr) != mMovementResults.end())
                    data.mActorRaw->setSimulationPosition(mMovementResults[data.mPtr]);
            }
            return mMovementResults;
        }

        // Remove actors that were deleted while the background thread was running
        for (auto& data : mActorsFrameData)
        {
            if (!data.mActor.lock())
                mMovementResults.erase(data.mPtr);
        }
        std::swap(mMovementResults, mPreviousMovementResults);

        // mMovementResults is shared between all workers instance
        // pre-allocate all nodes so that we don't need synchronization
        mMovementResults.clear();
        for (const auto& m : mActorsFrameData)
            mMovementResults[m.mPtr] = m.mPosition;

        lock.unlock();
        mHasJob.notify_all();
        return mPreviousMovementResults;
    }

    const PtrPositionList& PhysicsTaskScheduler::resetSimulation(const ActorMap& actors)
    {
        std::unique_lock lock(mSimulationMutex);
        mMovementResults.clear();
        mPreviousMovementResults.clear();
        mActorsFrameData.clear();

        for (const auto& [_, actor] : actors)
        {
            actor->resetPosition();
            actor->setStandingOnPtr(nullptr);
            mMovementResults[actor->getPtr()] = actor->getWorldPosition();
        }
        return mMovementResults;
    }

    void PhysicsTaskScheduler::rayTest(const btVector3& rayFromWorld, const btVector3& rayToWorld, btCollisionWorld::RayResultCallback& resultCallback) const
    {
        MaybeSharedLock lock(mCollisionWorldMutex, mThreadSafeBullet);
        mCollisionWorld->rayTest(rayFromWorld, rayToWorld, resultCallback);
    }

    void PhysicsTaskScheduler::convexSweepTest(const btConvexShape* castShape, const btTransform& from, const btTransform& to, btCollisionWorld::ConvexResultCallback& resultCallback) const
    {
        MaybeSharedLock lock(mCollisionWorldMutex, mThreadSafeBullet);
        mCollisionWorld->convexSweepTest(castShape, from, to, resultCallback);
    }

    void PhysicsTaskScheduler::contactTest(btCollisionObject* colObj, btCollisionWorld::ContactResultCallback& resultCallback)
    {
        std::shared_lock lock(mCollisionWorldMutex);
        mCollisionWorld->contactTest(colObj, resultCallback);
    }

    std::optional<btVector3> PhysicsTaskScheduler::getHitPoint(const btTransform& from, btCollisionObject* target)
    {
        MaybeSharedLock lock(mCollisionWorldMutex, mThreadSafeBullet);
        // target the collision object's world origin, this should be the center of the collision object
        btTransform rayTo;
        rayTo.setIdentity();
        rayTo.setOrigin(target->getWorldTransform().getOrigin());

        btCollisionWorld::ClosestRayResultCallback cb(from.getOrigin(), rayTo.getOrigin());

        mCollisionWorld->rayTestSingle(from, rayTo, target, target->getCollisionShape(), target->getWorldTransform(), cb);
        if (!cb.hasHit())
            // didn't hit the target. this could happen if point is already inside the collision box
            return std::nullopt;
        return {cb.m_hitPointWorld};
    }

    void PhysicsTaskScheduler::aabbTest(const btVector3& aabbMin, const btVector3& aabbMax, btBroadphaseAabbCallback& callback)
    {
        std::shared_lock lock(mCollisionWorldMutex);
        mCollisionWorld->getBroadphase()->aabbTest(aabbMin, aabbMax, callback);
    }

    void PhysicsTaskScheduler::getAabb(const btCollisionObject* obj, btVector3& min, btVector3& max)
    {
        std::shared_lock lock(mCollisionWorldMutex);
        obj->getCollisionShape()->getAabb(obj->getWorldTransform(), min, max);
    }

    void PhysicsTaskScheduler::setCollisionFilterMask(btCollisionObject* collisionObject, int collisionFilterMask)
    {
        std::unique_lock lock(mCollisionWorldMutex);
        collisionObject->getBroadphaseHandle()->m_collisionFilterMask = collisionFilterMask;
    }

    void PhysicsTaskScheduler::addCollisionObject(btCollisionObject* collisionObject, int collisionFilterGroup, int collisionFilterMask)
    {
        std::unique_lock lock(mCollisionWorldMutex);
        mCollisionWorld->addCollisionObject(collisionObject, collisionFilterGroup, collisionFilterMask);
    }

    void PhysicsTaskScheduler::removeCollisionObject(btCollisionObject* collisionObject)
    {
        std::unique_lock lock(mCollisionWorldMutex);
        mCollisionWorld->removeCollisionObject(collisionObject);
    }

    void PhysicsTaskScheduler::updateSingleAabb(std::weak_ptr<PtrHolder> ptr)
    {
        if (mDeferAabbUpdate)
        {
            std::unique_lock lock(mUpdateAabbMutex);
            mUpdateAabb.insert(std::move(ptr));
        }
        else
        {
            std::unique_lock lock(mCollisionWorldMutex);
            updatePtrAabb(ptr);
        }
    }

    bool PhysicsTaskScheduler::getLineOfSight(const std::weak_ptr<Actor>& actor1, const std::weak_ptr<Actor>& actor2)
    {
        std::unique_lock lock(mLOSCacheMutex);

        auto actorPtr1 = actor1.lock();
        auto actorPtr2 = actor2.lock();
        if (!actorPtr1 || !actorPtr2)
            return false;

        auto req = LOSRequest(actor1, actor2);
        auto result = std::find(mLOSCache.begin(), mLOSCache.end(), req);
        if (result == mLOSCache.end())
        {
            req.mResult = hasLineOfSight(actorPtr1.get(), actorPtr2.get());
            if (mLOSCacheExpiry >= 0)
                mLOSCache.push_back(req);
            return req.mResult;
        }
        result->mAge = 0;
        return result->mResult;
    }

    void PhysicsTaskScheduler::refreshLOSCache()
    {
        std::shared_lock lock(mLOSCacheMutex);
        int job = 0;
        int numLOS = mLOSCache.size();
        while ((job = mNextLOS.fetch_add(1, std::memory_order_relaxed)) < numLOS)
        {
            auto& req = mLOSCache[job];
            auto actorPtr1 = req.mActors[0].lock();
            auto actorPtr2 = req.mActors[1].lock();

            if (req.mAge++ > mLOSCacheExpiry || !actorPtr1 || !actorPtr2)
                req.mStale = true;
            else
                req.mResult = hasLineOfSight(actorPtr1.get(), actorPtr2.get());
        }

    }

    void PhysicsTaskScheduler::updateAabbs()
    {
        std::scoped_lock lock(mCollisionWorldMutex, mUpdateAabbMutex);
        std::for_each(mUpdateAabb.begin(), mUpdateAabb.end(),
            [this](const std::weak_ptr<PtrHolder>& ptr) { updatePtrAabb(ptr); });
        mUpdateAabb.clear();
    }

    void PhysicsTaskScheduler::updatePtrAabb(const std::weak_ptr<PtrHolder>& ptr)
    {
        if (const auto p = ptr.lock())
        {
            if (const auto actor = std::dynamic_pointer_cast<Actor>(p))
            {
                actor->updateCollisionObjectPosition();
                mCollisionWorld->updateSingleAabb(actor->getCollisionObject());
            }
            else if (const auto object = std::dynamic_pointer_cast<Object>(p))
            {
                object->commitPositionChange();
                mCollisionWorld->updateSingleAabb(object->getCollisionObject());
            }
            else if (const auto projectile = std::dynamic_pointer_cast<Projectile>(p))
            {
                projectile->commitPositionChange();
                mCollisionWorld->updateSingleAabb(projectile->getCollisionObject());
            }
        };
    }

    void PhysicsTaskScheduler::worker()
    {
        std::shared_lock lock(mSimulationMutex);
        while (!mQuit)
        {
            if (!mNewFrame)
                mHasJob.wait(lock, [&]() { return mQuit || mNewFrame; });

            if (mDeferAabbUpdate)
                mPreStepBarrier->wait();

            int job = 0;
            while (mRemainingSteps && (job = mNextJob.fetch_add(1, std::memory_order_relaxed)) < mNumJobs)
            {
                MaybeSharedLock lockColWorld(mCollisionWorldMutex, mThreadSafeBullet);
                if(const auto actor = mActorsFrameData[job].mActor.lock())
                    MovementSolver::move(mActorsFrameData[job], mPhysicsDt, mCollisionWorld.get(), *mWorldFrameData);
            }

            mPostStepBarrier->wait();

            if (!mRemainingSteps)
            {
                while ((job = mNextJob.fetch_add(1, std::memory_order_relaxed)) < mNumJobs)
                {
                    if(const auto actor = mActorsFrameData[job].mActor.lock())
                    {
                        auto& actorData = mActorsFrameData[job];
                        handleFall(actorData, mAdvanceSimulation);
                        mMovementResults[actorData.mPtr] = interpolateMovements(actorData, mTimeAccum, mPhysicsDt);
                    }
                }

                if (mLOSCacheExpiry >= 0)
                    refreshLOSCache();
                mPostSimBarrier->wait();
            }
        }
    }

    void PhysicsTaskScheduler::updateActorsPositions()
    {
        std::unique_lock lock(mCollisionWorldMutex);
        for (auto& actorData : mActorsFrameData)
        {
            if(const auto actor = actorData.mActor.lock())
            {
                bool positionChanged = actorData.mPosition != actorData.mActorRaw->getPosition();
                actorData.mActorRaw->setPosition(actorData.mPosition);
                if (positionChanged)
                {
                    actor->updateCollisionObjectPosition();
                    mCollisionWorld->updateSingleAabb(actor->getCollisionObject());
                }
            }
        }
    }

    bool PhysicsTaskScheduler::hasLineOfSight(const Actor* actor1, const Actor* actor2)
    {
        btVector3 pos1  = Misc::Convert::toBullet(actor1->getCollisionObjectPosition() + osg::Vec3f(0,0,actor1->getHalfExtents().z() * 0.9)); // eye level
        btVector3 pos2  = Misc::Convert::toBullet(actor2->getCollisionObjectPosition() + osg::Vec3f(0,0,actor2->getHalfExtents().z() * 0.9));

        btCollisionWorld::ClosestRayResultCallback resultCallback(pos1, pos2);
        resultCallback.m_collisionFilterGroup = 0xFF;
        resultCallback.m_collisionFilterMask = CollisionType_World|CollisionType_HeightMap|CollisionType_Door;

        MaybeSharedLock lockColWorld(mCollisionWorldMutex, mThreadSafeBullet);
        mCollisionWorld->rayTest(pos1, pos2, resultCallback);

        return !resultCallback.hasHit();
    }

    void PhysicsTaskScheduler::syncComputation()
    {
        while (mRemainingSteps--)
        {
            for (auto& actorData : mActorsFrameData)
                MovementSolver::move(actorData, mPhysicsDt, mCollisionWorld.get(), *mWorldFrameData);

            updateActorsPositions();
        }

        for (auto& actorData : mActorsFrameData)
        {
            handleFall(actorData, mAdvanceSimulation);
            mMovementResults[actorData.mPtr] = interpolateMovements(actorData, mTimeAccum, mPhysicsDt);
            updateMechanics(actorData);
        }
    }
}
