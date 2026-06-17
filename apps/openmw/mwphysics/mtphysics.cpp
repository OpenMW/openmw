#include "mtphysics.hpp"

#include <cassert>
#include <functional>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <stdexcept>
#include <variant>

#include <BulletCollision/BroadphaseCollision/btDbvtBroadphase.h>
#include <BulletCollision/CollisionShapes/btCollisionShape.h>
#include <LinearMath/btThreads.h>

#include <osg/Stats>

#include "components/debug/debuglog.hpp"
#include "components/misc/convert.hpp"
#include <components/misc/barrier.hpp>
#include <components/settings/values.hpp>

#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/creaturestats.hpp"

#include "../mwrender/bulletdebugdraw.hpp"

#include "../mwworld/class.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "actor.hpp"
#include "contacttestwrapper.h"
#include "movementsolver.hpp"
#include "object.hpp"
#include "physicssystem.hpp"
#include "projectile.hpp"

namespace MWPhysics
{
    namespace
    {
        template <class Mutex>
        std::optional<std::unique_lock<Mutex>> makeExclusiveLock(Mutex& mutex, LockingPolicy lockingPolicy)
        {
            if (lockingPolicy == LockingPolicy::NoLocks)
                return {};
            return std::unique_lock(mutex);
        }

        /// @brief A scoped lock that is either exclusive or inexistent depending on configuration
        template <class Mutex>
        class MaybeExclusiveLock
        {
        public:
            /// @param mutex a mutex
            /// @param threadCount decide wether the excluse lock will be taken
            explicit MaybeExclusiveLock(Mutex& mutex, LockingPolicy lockingPolicy)
                : mImpl(makeExclusiveLock(mutex, lockingPolicy))
            {
            }

        private:
            std::optional<std::unique_lock<Mutex>> mImpl;
        };

        template <class Mutex>
        std::optional<std::shared_lock<Mutex>> makeSharedLock(Mutex& mutex, LockingPolicy lockingPolicy)
        {
            if (lockingPolicy == LockingPolicy::NoLocks)
                return {};
            return std::shared_lock(mutex);
        }

        /// @brief A scoped lock that is either shared or inexistent depending on configuration
        template <class Mutex>
        class MaybeSharedLock
        {
        public:
            /// @param mutex a shared mutex
            /// @param threadCount decide wether the shared lock will be taken
            explicit MaybeSharedLock(Mutex& mutex, LockingPolicy lockingPolicy)
                : mImpl(makeSharedLock(mutex, lockingPolicy))
            {
            }

        private:
            std::optional<std::shared_lock<Mutex>> mImpl;
        };

        template <class Mutex>
        std::variant<std::monostate, std::unique_lock<Mutex>, std::shared_lock<Mutex>> makeLock(
            Mutex& mutex, LockingPolicy lockingPolicy)
        {
            switch (lockingPolicy)
            {
                case LockingPolicy::NoLocks:
                    return std::monostate{};
                case LockingPolicy::ExclusiveLocksOnly:
                    return std::unique_lock(mutex);
                case LockingPolicy::AllowSharedLocks:
                    return std::shared_lock(mutex);
            };

            throw std::runtime_error("Unsupported LockingPolicy: "
                + std::to_string(static_cast<std::underlying_type_t<LockingPolicy>>(lockingPolicy)));
        }

        /// @brief A scoped lock that is either shared, exclusive or inexistent depending on configuration
        template <class Mutex>
        class MaybeLock
        {
        public:
            /// @param mutex a shared mutex
            /// @param threadCount decide wether the lock will be shared, exclusive or inexistent
            explicit MaybeLock(Mutex& mutex, LockingPolicy lockingPolicy)
                : mImpl(makeLock(mutex, lockingPolicy))
            {
            }

        private:
            std::variant<std::monostate, std::unique_lock<Mutex>, std::shared_lock<Mutex>> mImpl;
        };
    }
}

namespace
{
    bool isUnderWater(const MWPhysics::ActorFrameData& actorData)
    {
        return actorData.mPosition.z() < actorData.mSwimLevel;
    }

    osg::Vec3f interpolateMovements(const MWPhysics::PtrHolder& ptr, float timeAccum, float physicsDt)
    {
        const float interpolationFactor = std::clamp(timeAccum / physicsDt, 0.0f, 1.0f);
        return ptr.getPosition() * interpolationFactor + ptr.getPreviousPosition() * (1.f - interpolationFactor);
    }

    using LockedActorSimulation
        = std::pair<std::shared_ptr<MWPhysics::Actor>, std::reference_wrapper<MWPhysics::ActorFrameData>>;
    using LockedProjectileSimulation
        = std::pair<std::shared_ptr<MWPhysics::Projectile>, std::reference_wrapper<MWPhysics::ProjectileFrameData>>;

    namespace Visitors
    {
        template <class Impl, template <class> class Lock>
        struct WithLockedPtr
        {
            const Impl& mImpl;
            std::shared_mutex& mCollisionWorldMutex;
            const MWPhysics::LockingPolicy mLockingPolicy;

            template <class Ptr, class FrameData>
            void operator()(MWPhysics::SimulationImpl<Ptr, FrameData>& sim) const
            {
                auto locked = sim.lock();
                if (!locked.has_value())
                    return;
                auto&& [ptr, frameData] = *std::move(locked);
                // Locked shared_ptr has to be destructed after releasing mCollisionWorldMutex to avoid
                // possible deadlock. Ptr destructor also acquires mCollisionWorldMutex.
                const std::pair arg(std::move(ptr), frameData);
                const Lock<std::shared_mutex> lock(mCollisionWorldMutex, mLockingPolicy);
                mImpl(arg);
            }
        };

        struct InitPosition
        {
            const btCollisionWorld* mCollisionWorld;
            void operator()(MWPhysics::ActorSimulation& sim) const
            {
                auto locked = sim.lock();
                if (!locked.has_value())
                    return;
                auto& [actor, frameDataRef] = *locked;
                auto& frameData = frameDataRef.get();
                frameData.mPosition = actor->applyOffsetChange();
                if (frameData.mWaterCollision && frameData.mPosition.z() < frameData.mWaterlevel
                    && actor->canMoveToWaterSurface(frameData.mWaterlevel, mCollisionWorld))
                {
                    const auto offset = osg::Vec3f(0, 0, frameData.mWaterlevel - frameData.mPosition.z());
                    MWBase::Environment::get().getWorld()->moveObjectBy(actor->getPtr(), offset, false);
                    frameData.mPosition = actor->applyOffsetChange();
                }
                actor->updateCollisionObjectPosition();
                frameData.mOldHeight = frameData.mPosition.z();
                const auto rotation = actor->getPtr().getRefData().getPosition().asRotationVec3();
                frameData.mRotation = osg::Vec2f(rotation.x(), rotation.z());
                frameData.mInertia = actor->getInertialForce();
                frameData.mStuckFrames = actor->getStuckFrames();
                frameData.mLastStuckPosition = actor->getLastStuckPosition();
            }
            void operator()(MWPhysics::ProjectileSimulation& /*sim*/) const {}
        };

        struct PreStep
        {
            btCollisionWorld* mCollisionWorld;
            void operator()(const LockedActorSimulation& sim) const
            {
                MWPhysics::MovementSolver::unstuck(sim.second, mCollisionWorld);
            }
            void operator()(const LockedProjectileSimulation& /*sim*/) const {}
        };

        struct UpdatePosition
        {
            btCollisionWorld* mCollisionWorld;
            void operator()(const LockedActorSimulation& sim) const
            {
                auto& [actor, frameDataRef] = sim;
                auto& frameData = frameDataRef.get();
                if (actor->setPosition(frameData.mPosition))
                {
                    frameData.mPosition = actor->getPosition(); // account for potential position change made by script
                    actor->updateCollisionObjectPosition();
                    mCollisionWorld->updateSingleAabb(actor->getCollisionObject());
                }
            }
            void operator()(const LockedProjectileSimulation& sim) const
            {
                auto& [proj, frameDataRef] = sim;
                auto& frameData = frameDataRef.get();
                proj->setPosition(frameData.mPosition);
                proj->updateCollisionObjectPosition();
                mCollisionWorld->updateSingleAabb(proj->getCollisionObject());
            }
        };

        struct Move
        {
            const float mPhysicsDt;
            const btCollisionWorld* mCollisionWorld;
            const MWPhysics::WorldFrameData& mWorldFrameData;
            void operator()(const LockedActorSimulation& sim) const
            {
                MWPhysics::MovementSolver::move(sim.second, mPhysicsDt, mCollisionWorld, mWorldFrameData);
            }
            void operator()(const LockedProjectileSimulation& sim) const
            {
                if (sim.first->isActive())
                    MWPhysics::MovementSolver::move(sim.second, mPhysicsDt, mCollisionWorld);
            }
        };

        struct Sync
        {
            const bool mAdvanceSimulation;
            const float mTimeAccum;
            const float mPhysicsDt;
            const MWPhysics::PhysicsTaskScheduler* scheduler;
            void operator()(MWPhysics::ActorSimulation& sim) const
            {
                auto locked = sim.lock();
                if (!locked.has_value())
                    return;
                auto& [actor, frameDataRef] = *locked;
                auto& frameData = frameDataRef.get();
                auto ptr = actor->getPtr();

                MWMechanics::CreatureStats& stats = ptr.getClass().getCreatureStats(ptr);
                const float heightDiff = frameData.mPosition.z() - frameData.mOldHeight;
                const bool isStillOnGround = (mAdvanceSimulation && frameData.mWasOnGround && frameData.mIsOnGround);

                if (isStillOnGround || frameData.mFlying || isUnderWater(frameData) || frameData.mSlowFall < 1)
                    stats.land(ptr == MWMechanics::getPlayer() && (frameData.mFlying || isUnderWater(frameData)));
                else if (heightDiff < 0)
                    stats.addToFallHeight(-heightDiff);

                actor->setSimulationPosition(::interpolateMovements(*actor, mTimeAccum, mPhysicsDt));
                actor->setLastStuckPosition(frameData.mLastStuckPosition);
                actor->setStuckFrames(frameData.mStuckFrames);
                if (mAdvanceSimulation)
                {
                    MWWorld::Ptr standingOn;
                    if (frameData.mStandingOn != nullptr)
                    {
                        auto* const ptrHolder
                            = static_cast<MWPhysics::PtrHolder*>(scheduler->getUserPointer(frameData.mStandingOn));
                        if (ptrHolder != nullptr)
                            standingOn = ptrHolder->getPtr();
                    }
                    actor->setStandingOnPtr(standingOn);
                    // the "on ground" state of an actor might have been updated by a traceDown, don't overwrite the
                    // change
                    if (actor->getOnGround() == frameData.mWasOnGround)
                        actor->setOnGround(frameData.mIsOnGround);
                    actor->setOnSlope(frameData.mIsOnSlope);
                    actor->setWalkingOnWater(frameData.mWalkingOnWater);
                    actor->setInertialForce(frameData.mInertia);
                }
            }
            void operator()(MWPhysics::ProjectileSimulation& sim) const
            {
                auto locked = sim.lock();
                if (!locked.has_value())
                    return;
                auto& [proj, frameData] = *locked;
                proj->setSimulationPosition(::interpolateMovements(*proj, mTimeAccum, mPhysicsDt));
            }
        };
    }
}

namespace MWPhysics
{
    namespace
    {
        unsigned getMaxBulletSupportedThreads()
        {
            auto broad = std::make_unique<btDbvtBroadphase>();
            assert(BT_MAX_THREAD_COUNT > 0);
            return std::min<unsigned>(broad->m_rayTestStacks.size(), BT_MAX_THREAD_COUNT - 1);
        }

        LockingPolicy detectLockingPolicy()
        {
            if (Settings::physics().mAsyncNumThreads < 1)
                return LockingPolicy::NoLocks;
            if (getMaxBulletSupportedThreads() > 1)
                return LockingPolicy::AllowSharedLocks;
            Log(Debug::Warning) << "Bullet was not compiled with multithreading support, 1 async thread will be used";
            return LockingPolicy::ExclusiveLocksOnly;
        }

        unsigned getNumThreads(LockingPolicy lockingPolicy)
        {
            switch (lockingPolicy)
            {
                case LockingPolicy::NoLocks:
                    return 0;
                case LockingPolicy::ExclusiveLocksOnly:
                    return 1;
                case LockingPolicy::AllowSharedLocks:
                    return static_cast<unsigned>(std::clamp<int>(
                        Settings::physics().mAsyncNumThreads, 0, static_cast<int>(getMaxBulletSupportedThreads())));
            }

            throw std::runtime_error("Unsupported LockingPolicy: "
                + std::to_string(static_cast<std::underlying_type_t<LockingPolicy>>(lockingPolicy)));
        }
    }

    class PhysicsTaskScheduler::WorkersSync
    {
    public:
        void waitForWorkers()
        {
            std::unique_lock lock(mWorkersDoneMutex);
            if (mFrameCounter != mWorkersFrameCounter)
                mWorkersDone.wait(lock);
        }

        void wakeUpWorkers()
        {
            const std::lock_guard lock(mHasJobMutex);
            ++mFrameCounter;
            mHasJob.notify_all();
        }

        void stopWorkers()
        {
            const std::lock_guard lock(mHasJobMutex);
            mShouldStop = true;
            mHasJob.notify_all();
        }

        void workIsDone()
        {
            const std::lock_guard lock(mWorkersDoneMutex);
            ++mWorkersFrameCounter;
            mWorkersDone.notify_all();
        }

        template <class F>
        void runWorker(F&& f) noexcept
        {
            std::size_t lastFrame = 0;
            std::unique_lock lock(mHasJobMutex);
            while (!mShouldStop)
            {
                mHasJob.wait(lock, [&] { return mShouldStop || mFrameCounter != lastFrame; });
                lastFrame = mFrameCounter;
                lock.unlock();
                f();
                lock.lock();
            }
        }

    private:
        std::size_t mWorkersFrameCounter = 0;
        std::condition_variable mWorkersDone;
        std::mutex mWorkersDoneMutex;
        std::condition_variable mHasJob;
        bool mShouldStop = false;
        std::size_t mFrameCounter = 0;
        std::mutex mHasJobMutex;
    };

    PhysicsTaskScheduler::PhysicsTaskScheduler(
        float physicsDt, btCollisionWorld* collisionWorld, MWRender::DebugDrawer* debugDrawer)
        : mDefaultPhysicsDt(physicsDt)
        , mPhysicsDt(physicsDt)
        , mTimeAccum(0.f)
        , mCollisionWorld(collisionWorld)
        , mDebugDrawer(debugDrawer)
        , mLockingPolicy(detectLockingPolicy())
        , mNumThreads(getNumThreads(mLockingPolicy))
        , mNumJobs(0)
        , mRemainingSteps(0)
        , mLOSCacheExpiry(Settings::physics().mLineofsightKeepInactiveCache)
        , mAdvanceSimulation(false)
        , mNextJob(0)
        , mNextLOS(0)
        , mFrameNumber(0)
        , mTimer(osg::Timer::instance())
        , mPrevStepCount(1)
        , mBudget(physicsDt)
        , mAsyncBudget(0.0f)
        , mBudgetCursor(0)
        , mAsyncStartTime(0)
        , mTimeBegin(0)
        , mTimeEnd(0)
        , mFrameStart(0)
        , mWorkersSync(mNumThreads >= 1 ? std::make_unique<WorkersSync>() : nullptr)
    {
        if (mNumThreads >= 1)
        {
            Log(Debug::Info) << "Using " << mNumThreads << " async physics threads";
            for (unsigned i = 0; i < mNumThreads; ++i)
                mThreads.emplace_back([&] { worker(); });
        }
        else
        {
            mLOSCacheExpiry = 0;
        }

        mPreStepBarrier = std::make_unique<Misc::Barrier>(mNumThreads);

        mPostStepBarrier = std::make_unique<Misc::Barrier>(mNumThreads);

        mPostSimBarrier = std::make_unique<Misc::Barrier>(mNumThreads);
    }

    PhysicsTaskScheduler::~PhysicsTaskScheduler()
    {
        waitForWorkers();
        {
            MaybeExclusiveLock lock(mSimulationMutex, mLockingPolicy);
            mNumJobs = 0;
            mRemainingSteps = 0;
        }
        if (mWorkersSync != nullptr)
            mWorkersSync->stopWorkers();
        for (auto& thread : mThreads)
            thread.join();
    }

    std::tuple<unsigned, float> PhysicsTaskScheduler::calculateStepConfig(float timeAccum) const
    {
        unsigned maxAllowedSteps = 2;
        unsigned numSteps = static_cast<unsigned>(timeAccum / mDefaultPhysicsDt);

        // adjust maximum step count based on whether we're likely physics bottlenecked or not
        // if maxAllowedSteps ends up higher than numSteps, we will not invoke delta time
        // if it ends up lower than numSteps, but greater than 1, we will run a number of true delta time physics steps
        // that we expect to be within budget if it ends up lower than numSteps and also 1, we will run a single delta
        // time physics step if we did not do this, and had a fixed step count limit, we would have an unnecessarily low
        // render framerate if we were only physics bottlenecked, and we would be unnecessarily invoking true delta time
        // if we were only render bottlenecked

        // get physics timing stats
        float budgetMeasurement = std::max(mBudget.get(), mAsyncBudget.get());
        // time spent per step in terms of the intended physics framerate
        budgetMeasurement /= mDefaultPhysicsDt;
        // ensure sane minimum value
        budgetMeasurement = std::max(0.00001f, budgetMeasurement);
        // we're spending almost or more than realtime per physics frame; limit to a single step
        if (budgetMeasurement > 0.95f)
            maxAllowedSteps = 1;
        // physics is fairly cheap; limit based on expense
        if (budgetMeasurement < 0.5f)
            maxAllowedSteps = static_cast<unsigned>(std::ceil(1.f / budgetMeasurement));
        // limit to a reasonable amount
        maxAllowedSteps = std::min(10u, maxAllowedSteps);

        // fall back to delta time for this frame if fixed timestep physics would fall behind
        float actualDelta = mDefaultPhysicsDt;
        if (numSteps > maxAllowedSteps)
        {
            numSteps = maxAllowedSteps;
            // ensure that we do not simulate a frame ahead when doing delta time; this reduces stutter and latency
            // this causes interpolation to 100% use the most recent physics result when true delta time is happening
            // and we deliberately simulate up to exactly the timestamp that we want to render
            actualDelta = timeAccum / float(numSteps + 1);
            // actually: if this results in a per-step delta less than the target physics steptime, clamp it
            // this might reintroduce some stutter, but only comes into play in obscure cases
            // (because numSteps is originally based on mDefaultPhysicsDt, this won't cause us to overrun)
            actualDelta = std::max(actualDelta, mDefaultPhysicsDt);
        }

        return std::make_tuple(numSteps, actualDelta);
    }

    void PhysicsTaskScheduler::applyQueuedMovements(float& timeAccum, std::vector<Simulation>& simulations,
        osg::Timer_t frameStart, unsigned int frameNumber, osg::Stats& stats)
    {
        assert(mSimulations != &simulations);

        waitForWorkers();
        prepareWork(timeAccum, simulations, frameStart, frameNumber, stats);
        if (mWorkersSync != nullptr)
            mWorkersSync->wakeUpWorkers();
    }

    void PhysicsTaskScheduler::prepareWork(float& timeAccum, std::vector<Simulation>& simulations,
        osg::Timer_t frameStart, unsigned int frameNumber, osg::Stats& stats)
    {
        // This function run in the main thread.
        // While the mSimulationMutex is held, background physics threads can't run.

        MaybeExclusiveLock lock(mSimulationMutex, mLockingPolicy);

        auto timeStart = mTimer->tick();

        // start by finishing previous background computation
        if (mNumThreads != 0)
        {
            syncWithMainThread();

            if (mAdvanceSimulation)
                mAsyncBudget.update(mTimer->delta_s(mAsyncStartTime, mTimeEnd), mPrevStepCount, mBudgetCursor);
            updateStats(frameStart, frameNumber, stats);
        }

        auto [numSteps, newDelta] = calculateStepConfig(timeAccum);
        timeAccum -= numSteps * newDelta;

        // init
        const Visitors::InitPosition vis{ mCollisionWorld };
        for (auto& sim : simulations)
        {
            std::visit(vis, sim);
        }
        mPrevStepCount = numSteps;
        mRemainingSteps = numSteps;
        mTimeAccum = timeAccum;
        mPhysicsDt = newDelta;
        mSimulations = &simulations;
        mAdvanceSimulation = (mRemainingSteps != 0);
        mNumJobs = static_cast<int>(mSimulations->size());
        mNextLOS.store(0, std::memory_order_relaxed);
        mNextJob.store(0, std::memory_order_release);

        if (mAdvanceSimulation)
            mWorldFrameData = std::make_unique<WorldFrameData>();

        if (mAdvanceSimulation)
            mBudgetCursor += 1;

        if (mNumThreads == 0)
        {
            doSimulation();
            syncWithMainThread();
            if (mAdvanceSimulation)
                mBudget.update(mTimer->delta_s(timeStart, mTimer->tick()), numSteps, mBudgetCursor);
            return;
        }

        mAsyncStartTime = mTimer->tick();
        if (mAdvanceSimulation)
            mBudget.update(mTimer->delta_s(timeStart, mTimer->tick()), 1, mBudgetCursor);
    }

    void PhysicsTaskScheduler::resetSimulation(const ActorMap& actors)
    {
        waitForWorkers();
        MaybeExclusiveLock lock(mSimulationMutex, mLockingPolicy);
        mBudget.reset(mDefaultPhysicsDt);
        mAsyncBudget.reset(0.0f);
        if (mSimulations != nullptr)
        {
            mSimulations->clear();
            mSimulations = nullptr;
        }
        for (const auto& [_, actor] : actors)
        {
            actor->updatePosition();
            actor->updateCollisionObjectPosition();
        }
    }

    void PhysicsTaskScheduler::rayTest(const btVector3& rayFromWorld, const btVector3& rayToWorld,
        btCollisionWorld::RayResultCallback& resultCallback) const
    {
        MaybeLock lock(mCollisionWorldMutex, mLockingPolicy);
        mCollisionWorld->rayTest(rayFromWorld, rayToWorld, resultCallback);
    }

    void PhysicsTaskScheduler::convexSweepTest(const btConvexShape* castShape, const btTransform& from,
        const btTransform& to, btCollisionWorld::ConvexResultCallback& resultCallback) const
    {
        MaybeLock lock(mCollisionWorldMutex, mLockingPolicy);
        mCollisionWorld->convexSweepTest(castShape, from, to, resultCallback);
    }

    void PhysicsTaskScheduler::contactTest(
        btCollisionObject* colObj, btCollisionWorld::ContactResultCallback& resultCallback)
    {
        MaybeSharedLock lock(mCollisionWorldMutex, mLockingPolicy);
        ContactTestWrapper::contactTest(mCollisionWorld, colObj, resultCallback);
    }

    std::optional<btVector3> PhysicsTaskScheduler::getHitPoint(const btTransform& from, btCollisionObject* target)
    {
        MaybeLock lock(mCollisionWorldMutex, mLockingPolicy);
        // target the collision object's world origin, this should be the center of the collision object
        btTransform rayTo;
        rayTo.setIdentity();
        rayTo.setOrigin(target->getWorldTransform().getOrigin());

        btCollisionWorld::ClosestRayResultCallback cb(from.getOrigin(), rayTo.getOrigin());

        mCollisionWorld->rayTestSingle(
            from, rayTo, target, target->getCollisionShape(), target->getWorldTransform(), cb);
        if (!cb.hasHit())
            // didn't hit the target. this could happen if point is already inside the collision box
            return std::nullopt;
        return { cb.m_hitPointWorld };
    }

    void PhysicsTaskScheduler::aabbTest(
        const btVector3& aabbMin, const btVector3& aabbMax, btBroadphaseAabbCallback& callback)
    {
        MaybeSharedLock lock(mCollisionWorldMutex, mLockingPolicy);
        mCollisionWorld->getBroadphase()->aabbTest(aabbMin, aabbMax, callback);
    }

    void PhysicsTaskScheduler::getAabb(const btCollisionObject* obj, btVector3& min, btVector3& max)
    {
        MaybeSharedLock lock(mCollisionWorldMutex, mLockingPolicy);
        obj->getCollisionShape()->getAabb(obj->getWorldTransform(), min, max);
    }

    void PhysicsTaskScheduler::setCollisionFilterMask(btCollisionObject* collisionObject, int collisionFilterMask)
    {
        MaybeExclusiveLock lock(mCollisionWorldMutex, mLockingPolicy);
        collisionObject->getBroadphaseHandle()->m_collisionFilterMask = collisionFilterMask;
    }

    void PhysicsTaskScheduler::addCollisionObject(
        btCollisionObject* collisionObject, int collisionFilterGroup, int collisionFilterMask)
    {
        MaybeExclusiveLock lock(mCollisionWorldMutex, mLockingPolicy);
        mCollisionObjects.insert(collisionObject);
        mCollisionWorld->addCollisionObject(collisionObject, collisionFilterGroup, collisionFilterMask);
    }

    void PhysicsTaskScheduler::removeCollisionObject(btCollisionObject* collisionObject)
    {
        MaybeExclusiveLock lock(mCollisionWorldMutex, mLockingPolicy);
        mCollisionObjects.erase(collisionObject);
        mCollisionWorld->removeCollisionObject(collisionObject);
    }

    void PhysicsTaskScheduler::updateSingleAabb(const std::shared_ptr<PtrHolder>& ptr, bool immediate)
    {
        if (immediate || mNumThreads == 0)
        {
            updatePtrAabb(ptr);
        }
        else
        {
            MaybeExclusiveLock lock(mUpdateAabbMutex, mLockingPolicy);
            mUpdateAabb.insert(ptr);
        }
    }

    bool PhysicsTaskScheduler::getLineOfSight(
        const std::shared_ptr<Actor>& actor1, const std::shared_ptr<Actor>& actor2)
    {
        MaybeExclusiveLock lock(mLOSCacheMutex, mLockingPolicy);

        auto req = LOSRequest(actor1, actor2);
        auto result = std::find(mLOSCache.begin(), mLOSCache.end(), req);
        if (result == mLOSCache.end())
        {
            req.mResult = hasLineOfSight(actor1.get(), actor2.get());
            mLOSCache.push_back(req);
            return req.mResult;
        }
        result->mAge = 0;
        return result->mResult;
    }

    void PhysicsTaskScheduler::refreshLOSCache()
    {
        MaybeSharedLock lock(mLOSCacheMutex, mLockingPolicy);
        int job = 0;
        int numLOS = static_cast<int>(mLOSCache.size());
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
        MaybeExclusiveLock lock(mUpdateAabbMutex, mLockingPolicy);
        std::for_each(mUpdateAabb.begin(), mUpdateAabb.end(), [this](const std::weak_ptr<PtrHolder>& ptr) {
            auto p = ptr.lock();
            if (p != nullptr)
                updatePtrAabb(p);
        });
        mUpdateAabb.clear();
    }

    void PhysicsTaskScheduler::updatePtrAabb(const std::shared_ptr<PtrHolder>& ptr)
    {
        MaybeExclusiveLock lock(mCollisionWorldMutex, mLockingPolicy);
        if (const auto actor = std::dynamic_pointer_cast<Actor>(ptr))
        {
            actor->updateCollisionObjectPosition();
            mCollisionWorld->updateSingleAabb(actor->getCollisionObject());
        }
        else if (const auto object = std::dynamic_pointer_cast<Object>(ptr))
        {
            object->commitPositionChange();
            mCollisionWorld->updateSingleAabb(object->getCollisionObject());
        }
        else if (const auto projectile = std::dynamic_pointer_cast<Projectile>(ptr))
        {
            projectile->updateCollisionObjectPosition();
            mCollisionWorld->updateSingleAabb(projectile->getCollisionObject());
        }
    }

    void PhysicsTaskScheduler::worker()
    {
        mWorkersSync->runWorker([this] {
            std::shared_lock lock(mSimulationMutex);
            doSimulation();
        });
    }

    void PhysicsTaskScheduler::updateActorsPositions()
    {
        const Visitors::UpdatePosition impl{ mCollisionWorld };
        const Visitors::WithLockedPtr<Visitors::UpdatePosition, MaybeExclusiveLock> vis{ impl, mCollisionWorldMutex,
            mLockingPolicy };
        for (Simulation& sim : *mSimulations)
            std::visit(vis, sim);
    }

    bool PhysicsTaskScheduler::hasLineOfSight(const Actor* actor1, const Actor* actor2)
    {
        btVector3 pos1 = Misc::Convert::toBullet(
            actor1->getCollisionObjectPosition() + osg::Vec3f(0, 0, actor1->getHalfExtents().z() * 0.9f)); // eye level
        btVector3 pos2 = Misc::Convert::toBullet(
            actor2->getCollisionObjectPosition() + osg::Vec3f(0, 0, actor2->getHalfExtents().z() * 0.9f));

        btCollisionWorld::ClosestRayResultCallback resultCallback(pos1, pos2);
        resultCallback.m_collisionFilterGroup = CollisionType_AnyPhysical;
        resultCallback.m_collisionFilterMask = CollisionType_World | CollisionType_HeightMap | CollisionType_Door;

        MaybeLock lockColWorld(mCollisionWorldMutex, mLockingPolicy);
        mCollisionWorld->rayTest(pos1, pos2, resultCallback);

        return !resultCallback.hasHit();
    }

    void PhysicsTaskScheduler::doSimulation()
    {
        while (mRemainingSteps)
        {
            mPreStepBarrier->wait([this] { afterPreStep(); });
            int job = 0;
            const Visitors::Move impl{ mPhysicsDt, mCollisionWorld, *mWorldFrameData };
            const Visitors::WithLockedPtr<Visitors::Move, MaybeLock> vis{ impl, mCollisionWorldMutex, mLockingPolicy };
            while ((job = mNextJob.fetch_add(1, std::memory_order_relaxed)) < mNumJobs)
                std::visit(vis, (*mSimulations)[job]);

            mPostStepBarrier->wait([this] { afterPostStep(); });
        }

        refreshLOSCache();
        mPostSimBarrier->wait([this] { afterPostSim(); });
    }

    void PhysicsTaskScheduler::updateStats(osg::Timer_t frameStart, unsigned int frameNumber, osg::Stats& stats)
    {
        if (!stats.collectStats("engine"))
            return;
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

    void PhysicsTaskScheduler::debugDraw()
    {
        MaybeSharedLock lock(mCollisionWorldMutex, mLockingPolicy);
        mDebugDrawer->step();
    }

    void* PhysicsTaskScheduler::getUserPointer(const btCollisionObject* object) const
    {
        auto it = mCollisionObjects.find(object);
        if (it == mCollisionObjects.end())
            return nullptr;
        return (*it)->getUserPointer();
    }

    void PhysicsTaskScheduler::releaseSharedStates()
    {
        waitForWorkers();
        std::scoped_lock lock(mSimulationMutex, mUpdateAabbMutex);
        if (mSimulations != nullptr)
        {
            mSimulations->clear();
            mSimulations = nullptr;
        }
        mUpdateAabb.clear();
    }

    void PhysicsTaskScheduler::afterPreStep()
    {
        updateAabbs();
        if (!mRemainingSteps)
            return;
        const Visitors::PreStep impl{ mCollisionWorld };
        const Visitors::WithLockedPtr<Visitors::PreStep, MaybeExclusiveLock> vis{ impl, mCollisionWorldMutex,
            mLockingPolicy };
        for (auto& sim : *mSimulations)
            std::visit(vis, sim);
    }

    void PhysicsTaskScheduler::afterPostStep()
    {
        if (mRemainingSteps)
        {
            --mRemainingSteps;
            updateActorsPositions();
        }
        mNextJob.store(0, std::memory_order_release);
    }

    void PhysicsTaskScheduler::afterPostSim()
    {
        {
            MaybeExclusiveLock lock(mLOSCacheMutex, mLockingPolicy);
            mLOSCache.erase(
                std::remove_if(mLOSCache.begin(), mLOSCache.end(), [](const LOSRequest& req) { return req.mStale; }),
                mLOSCache.end());
        }
        mTimeEnd = mTimer->tick();
        if (mWorkersSync != nullptr)
            mWorkersSync->workIsDone();
    }

    void PhysicsTaskScheduler::syncWithMainThread()
    {
        if (mSimulations == nullptr)
            return;
        const Visitors::Sync vis{ mAdvanceSimulation, mTimeAccum, mPhysicsDt, this };
        for (auto& sim : *mSimulations)
            std::visit(vis, sim);
        mSimulations->clear();
        mSimulations = nullptr;
    }

    // Attempt to acquire unique lock on mSimulationMutex while not all worker
    // threads are holding shared lock but will have to may lead to a deadlock because
    // C++ standard does not guarantee priority for exclusive and shared locks
    // for std::shared_mutex. For example microsoft STL implementation points out
    // for the absence of such priority:
    // https://docs.microsoft.com/en-us/windows/win32/sync/slim-reader-writer--srw--locks
    void PhysicsTaskScheduler::waitForWorkers()
    {
        if (mWorkersSync != nullptr)
            mWorkersSync->waitForWorkers();
    }
}
