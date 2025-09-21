#include "physicssystem.hpp"

#include <algorithm>
#include <memory>
#include <vector>

#include <osg/Group>
#include <osg/Stats>
#include <osg/Timer>

#include <BulletCollision/BroadphaseCollision/btDbvtBroadphase.h>
#include <BulletCollision/CollisionDispatch/btCollisionObject.h>
#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>
#include <BulletCollision/CollisionDispatch/btDefaultCollisionConfiguration.h>
#include <BulletCollision/CollisionShapes/btConeShape.h>
#include <BulletCollision/CollisionShapes/btSphereShape.h>
#include <BulletCollision/CollisionShapes/btStaticPlaneShape.h>

#include <LinearMath/btQuickprof.h>
#include <LinearMath/btVector3.h>

#include <components/debug/debuglog.hpp>
#include <components/esm3/loadgmst.hpp>
#include <components/esm3/loadmgef.hpp>
#include <components/misc/convert.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/misc/strings/conversion.hpp>
#include <components/resource/bulletshapemanager.hpp>
#include <components/resource/resourcesystem.hpp>
#include <components/settings/values.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/movement.hpp"

#include "../mwworld/cellstore.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/player.hpp"

#include "../mwrender/bulletdebugdraw.hpp"

#include "../mwworld/class.hpp"

#include "actor.hpp"
#include "collisiontype.hpp"

#include "closestnotmerayresultcallback.hpp"
#include "contacttestresultcallback.hpp"
#include "hasspherecollisioncallback.hpp"
#include "heightfield.hpp"
#include "movementsolver.hpp"
#include "mtphysics.hpp"
#include "object.hpp"
#include "projectile.hpp"

namespace
{
    void handleJump(const MWWorld::Ptr& ptr)
    {
        if (!ptr.getClass().isActor())
            return;
        if (ptr.getClass().getMovementSettings(ptr).mPosition[2] == 0)
            return;
        const bool isPlayer = (ptr == MWMechanics::getPlayer());
        // Advance acrobatics and set flag for GetPCJumping
        if (isPlayer)
        {
            ptr.getClass().skillUsageSucceeded(ptr, ESM::Skill::Acrobatics, ESM::Skill::Acrobatics_Jump);
            MWBase::Environment::get().getWorld()->getPlayer().setJumping(true);
        }

        // Decrease fatigue
        if (!isPlayer || !MWBase::Environment::get().getWorld()->getGodModeState())
        {
            const MWWorld::Store<ESM::GameSetting>& gmst
                = MWBase::Environment::get().getESMStore()->get<ESM::GameSetting>();
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

}

namespace MWPhysics
{
    PhysicsSystem::PhysicsSystem(Resource::ResourceSystem* resourceSystem, osg::ref_ptr<osg::Group> parentNode)
        : mPhysicsDt(1.f / 60.f)
        , mShapeManager(std::make_unique<Resource::BulletShapeManager>(resourceSystem->getVFS(),
              resourceSystem->getSceneManager(), resourceSystem->getNifFileManager(),
              Settings::cells().mCacheExpiryDelay))
        , mResourceSystem(resourceSystem)
        , mDebugDrawEnabled(false)
        , mTimeAccum(0.0f)
        , mProjectileId(0)
        , mWaterHeight(0)
        , mWaterEnabled(false)
        , mParentNode(std::move(parentNode))
    {
        mResourceSystem->addResourceManager(mShapeManager.get());

        mCollisionConfiguration = std::make_unique<btDefaultCollisionConfiguration>();
        mDispatcher = std::make_unique<btCollisionDispatcher>(mCollisionConfiguration.get());
        mBroadphase = std::make_unique<btDbvtBroadphase>();

        mCollisionWorld
            = std::make_unique<btCollisionWorld>(mDispatcher.get(), mBroadphase.get(), mCollisionConfiguration.get());

        // Don't update AABBs of all objects every frame. Most objects in MW are static, so we don't need this.
        // Should a "static" object ever be moved, we have to update its AABB manually using
        // DynamicsWorld::updateSingleAabb.
        mCollisionWorld->setForceUpdateAllAabbs(false);

        // Check if a user decided to override a physics system FPS
        if (const char* env = getenv("OPENMW_PHYSICS_FPS"))
        {
            if (const auto physFramerate = Misc::StringUtils::toNumeric<float>(env);
                physFramerate.has_value() && *physFramerate > 0)
            {
                mPhysicsDt = 1.f / *physFramerate;
                Log(Debug::Warning) << "Warning: using custom physics framerate (" << *physFramerate << " FPS).";
            }
        }

        mDebugDrawer = std::make_unique<MWRender::DebugDrawer>(mParentNode, mCollisionWorld.get(), mDebugDrawEnabled);
        mTaskScheduler = std::make_unique<PhysicsTaskScheduler>(mPhysicsDt, mCollisionWorld.get(), mDebugDrawer.get());
    }

    PhysicsSystem::~PhysicsSystem()
    {
        mResourceSystem->removeResourceManager(mShapeManager.get());

        if (mWaterCollisionObject)
            mTaskScheduler->removeCollisionObject(mWaterCollisionObject.get());

        mTaskScheduler->releaseSharedStates();
        mHeightFields.clear();
        mObjects.clear();
        mActors.clear();
        mProjectiles.clear();
    }

    Resource::BulletShapeManager* PhysicsSystem::getShapeManager()
    {
        return mShapeManager.get();
    }

    bool PhysicsSystem::toggleDebugRendering()
    {
        mDebugDrawEnabled = !mDebugDrawEnabled;

        mCollisionWorld->setDebugDrawer(mDebugDrawEnabled ? mDebugDrawer.get() : nullptr);
        mDebugDrawer->setDebugMode(mDebugDrawEnabled);
        return mDebugDrawEnabled;
    }

    void PhysicsSystem::markAsNonSolid(const MWWorld::ConstPtr& ptr)
    {
        ObjectMap::iterator found = mObjects.find(ptr.mRef);
        if (found == mObjects.end())
            return;

        found->second->setSolid(false);
    }

    bool PhysicsSystem::isOnSolidGround(const MWWorld::Ptr& actor) const
    {
        const Actor* physactor = getActor(actor);
        if (!physactor || !physactor->getOnGround() || !physactor->getCollisionMode())
            return false;

        const auto obj = physactor->getStandingOnPtr();
        if (obj.isEmpty())
            return true; // assume standing on terrain (which is a non-object, so not collision tracked)

        ObjectMap::const_iterator foundObj = mObjects.find(obj.mRef);
        if (foundObj == mObjects.end())
            return false;

        if (!foundObj->second->isSolid())
            return false;

        return true;
    }

    RayCastingResult PhysicsSystem::castRay(const osg::Vec3f& from, const osg::Vec3f& to,
        const std::vector<MWWorld::ConstPtr>& ignore, const std::vector<MWWorld::Ptr>& targets, int mask,
        int group) const
    {
        if (from == to)
        {
            RayCastingResult result;
            result.mHit = false;
            return result;
        }
        btVector3 btFrom = Misc::Convert::toBullet(from);
        btVector3 btTo = Misc::Convert::toBullet(to);

        std::vector<const btCollisionObject*> ignoreList;
        std::vector<const btCollisionObject*> targetCollisionObjects;

        for (const auto& ptr : ignore)
        {
            if (!ptr.isEmpty())
            {
                const Actor* actor = getActor(ptr);
                if (actor)
                    ignoreList.push_back(actor->getCollisionObject());
                else
                {
                    const Object* object = getObject(ptr);
                    if (object)
                        ignoreList.push_back(object->getCollisionObject());
                }
            }
        }

        if (!targets.empty())
        {
            for (const MWWorld::Ptr& target : targets)
            {
                const Actor* actor = getActor(target);
                if (actor)
                    targetCollisionObjects.push_back(actor->getCollisionObject());
            }
        }

        ClosestNotMeRayResultCallback resultCallback(ignoreList, targetCollisionObjects, btFrom, btTo);
        resultCallback.m_collisionFilterGroup = group;
        resultCallback.m_collisionFilterMask = mask;

        mTaskScheduler->rayTest(btFrom, btTo, resultCallback);

        RayCastingResult result;
        result.mHit = resultCallback.hasHit();
        if (resultCallback.hasHit())
        {
            result.mHitPos = Misc::Convert::toOsg(resultCallback.m_hitPointWorld);
            result.mHitNormal = Misc::Convert::toOsg(resultCallback.m_hitNormalWorld);
            if (PtrHolder* ptrHolder = static_cast<PtrHolder*>(resultCallback.m_collisionObject->getUserPointer()))
                result.mHitObject = ptrHolder->getPtr();
        }
        return result;
    }

    RayCastingResult PhysicsSystem::castSphere(
        const osg::Vec3f& from, const osg::Vec3f& to, float radius, int mask, int group) const
    {
        btCollisionWorld::ClosestConvexResultCallback callback(
            Misc::Convert::toBullet(from), Misc::Convert::toBullet(to));
        callback.m_collisionFilterGroup = group;
        callback.m_collisionFilterMask = mask;

        btSphereShape shape(radius);
        const btQuaternion btrot = btQuaternion::getIdentity();

        mTaskScheduler->convexSweepTest(&shape, btTransform(btrot, Misc::Convert::toBullet(from)),
            btTransform(btrot, Misc::Convert::toBullet(to)), callback);

        RayCastingResult result;
        result.mHit = callback.hasHit();
        if (result.mHit)
        {
            result.mHitPos = Misc::Convert::toOsg(callback.m_hitPointWorld);
            result.mHitNormal = Misc::Convert::toOsg(callback.m_hitNormalWorld);
            if (auto* ptrHolder = static_cast<PtrHolder*>(callback.m_hitCollisionObject->getUserPointer()))
                result.mHitObject = ptrHolder->getPtr();
        }
        return result;
    }

    bool PhysicsSystem::getLineOfSight(const MWWorld::ConstPtr& actor1, const MWWorld::ConstPtr& actor2) const
    {
        if (actor1 == actor2)
            return true;

        const auto it1 = mActors.find(actor1.mRef);
        const auto it2 = mActors.find(actor2.mRef);
        if (it1 == mActors.end() || it2 == mActors.end())
            return false;

        return mTaskScheduler->getLineOfSight(it1->second, it2->second);
    }

    bool PhysicsSystem::isOnGround(const MWWorld::Ptr& actor)
    {
        Actor* physactor = getActor(actor);
        return physactor && physactor->getOnGround() && physactor->getCollisionMode();
    }

    bool PhysicsSystem::canMoveToWaterSurface(const MWWorld::ConstPtr& actor, const float waterlevel)
    {
        const auto* physactor = getActor(actor);
        return physactor && physactor->canMoveToWaterSurface(waterlevel, mCollisionWorld.get());
    }

    osg::Vec3f PhysicsSystem::getHalfExtents(const MWWorld::ConstPtr& actor) const
    {
        const Actor* physactor = getActor(actor);
        if (physactor)
            return physactor->getHalfExtents();
        else
            return osg::Vec3f();
    }

    osg::Vec3f PhysicsSystem::getOriginalHalfExtents(const MWWorld::ConstPtr& actor) const
    {
        if (const Actor* physactor = getActor(actor))
            return physactor->getOriginalHalfExtents();
        else
            return osg::Vec3f();
    }

    osg::Vec3f PhysicsSystem::getRenderingHalfExtents(const MWWorld::ConstPtr& actor) const
    {
        const Actor* physactor = getActor(actor);
        if (physactor)
            return physactor->getRenderingHalfExtents();
        else
            return osg::Vec3f();
    }

    osg::BoundingBox PhysicsSystem::getBoundingBox(const MWWorld::ConstPtr& object) const
    {
        const Object* physobject = getObject(object);
        if (!physobject)
            return osg::BoundingBox();
        btVector3 min, max;
        mTaskScheduler->getAabb(physobject->getCollisionObject(), min, max);
        return osg::BoundingBox(Misc::Convert::toOsg(min), Misc::Convert::toOsg(max));
    }

    osg::Vec3f PhysicsSystem::getCollisionObjectPosition(const MWWorld::ConstPtr& actor) const
    {
        const Actor* physactor = getActor(actor);
        if (physactor)
            return physactor->getCollisionObjectPosition();
        else
            return osg::Vec3f();
    }

    std::vector<ContactPoint> PhysicsSystem::getCollisionsPoints(
        const MWWorld::ConstPtr& ptr, int collisionGroup, int collisionMask) const
    {
        btCollisionObject* me = nullptr;

        auto found = mObjects.find(ptr.mRef);
        if (found != mObjects.end())
            me = found->second->getCollisionObject();
        else
            return {};

        ContactTestResultCallback resultCallback(me);
        resultCallback.m_collisionFilterGroup = collisionGroup;
        resultCallback.m_collisionFilterMask = collisionMask;
        mTaskScheduler->contactTest(me, resultCallback);
        return resultCallback.mResult;
    }

    std::vector<MWWorld::Ptr> PhysicsSystem::getCollisions(
        const MWWorld::ConstPtr& ptr, int collisionGroup, int collisionMask) const
    {
        std::vector<MWWorld::Ptr> actors;
        for (auto& [actor, point, normal] : getCollisionsPoints(ptr, collisionGroup, collisionMask))
            actors.emplace_back(actor);
        return actors;
    }

    osg::Vec3f PhysicsSystem::traceDown(const MWWorld::Ptr& ptr, const osg::Vec3f& position, float maxHeight)
    {
        ActorMap::iterator found = mActors.find(ptr.mRef);
        if (found == mActors.end())
            return ptr.getRefData().getPosition().asVec3();
        return MovementSolver::traceDown(ptr, position, found->second.get(), mCollisionWorld.get(), maxHeight);
    }

    void PhysicsSystem::addHeightField(
        const float* heights, int x, int y, int size, int verts, float minH, float maxH, const osg::Object* holdObject)
    {
        mHeightFields[std::make_pair(x, y)]
            = std::make_unique<HeightField>(heights, x, y, size, verts, minH, maxH, holdObject, mTaskScheduler.get());
    }

    void PhysicsSystem::removeHeightField(int x, int y)
    {
        HeightFieldMap::iterator heightfield = mHeightFields.find(std::make_pair(x, y));
        if (heightfield != mHeightFields.end())
            mHeightFields.erase(heightfield);
    }

    const HeightField* PhysicsSystem::getHeightField(int x, int y) const
    {
        const auto heightField = mHeightFields.find(std::make_pair(x, y));
        if (heightField == mHeightFields.end())
            return nullptr;
        return heightField->second.get();
    }

    void PhysicsSystem::addObject(
        const MWWorld::Ptr& ptr, VFS::Path::NormalizedView mesh, osg::Quat rotation, int collisionType)
    {
        if (ptr.mRef->mData.mPhysicsPostponed)
            return;

        const VFS::Path::Normalized animationMesh = ptr.getClass().useAnim()
            ? Misc::ResourceHelpers::correctActorModelPath(mesh, mResourceSystem->getVFS())
            : VFS::Path::Normalized(mesh);
        osg::ref_ptr<Resource::BulletShapeInstance> shapeInstance = mShapeManager->getInstance(animationMesh);
        if (!shapeInstance || !shapeInstance->mCollisionShape)
            return;

        assert(!getObject(ptr));

        // Override collision type based on shape content.
        switch (shapeInstance->mVisualCollisionType)
        {
            case Resource::VisualCollisionType::None:
                break;
            case Resource::VisualCollisionType::Default:
                collisionType = CollisionType_VisualOnly;
                break;
            case Resource::VisualCollisionType::Camera:
                collisionType = CollisionType_CameraOnly;
                break;
        }

        auto obj = std::make_shared<Object>(ptr, shapeInstance, rotation, collisionType, mTaskScheduler.get());
        mObjects.emplace(ptr.mRef, obj);

        if (obj->isAnimated())
            mAnimatedObjects.emplace(obj.get(), false);
    }

    void PhysicsSystem::remove(const MWWorld::Ptr& ptr)
    {
        if (auto foundObject = mObjects.find(ptr.mRef); foundObject != mObjects.end())
        {
            mAnimatedObjects.erase(foundObject->second.get());

            mObjects.erase(foundObject);
        }
        else if (auto foundActor = mActors.find(ptr.mRef); foundActor != mActors.end())
        {
            mActors.erase(foundActor);
        }
    }

    void PhysicsSystem::removeProjectile(const int projectileId)
    {
        ProjectileMap::iterator foundProjectile = mProjectiles.find(projectileId);
        if (foundProjectile != mProjectiles.end())
            mProjectiles.erase(foundProjectile);
    }

    void PhysicsSystem::updatePtr(const MWWorld::Ptr& old, const MWWorld::Ptr& updated)
    {
        if (auto foundObject = mObjects.find(old.mRef); foundObject != mObjects.end())
            foundObject->second->updatePtr(updated);
        else if (auto foundActor = mActors.find(old.mRef); foundActor != mActors.end())
            foundActor->second->updatePtr(updated);

        for (auto& [_, actor] : mActors)
        {
            if (actor->getStandingOnPtr() == old)
                actor->setStandingOnPtr(updated);
        }

        for (auto& [_, projectile] : mProjectiles)
        {
            if (projectile->getCaster() == old)
                projectile->setCaster(updated);
        }
    }

    Actor* PhysicsSystem::getActor(const MWWorld::Ptr& ptr)
    {
        ActorMap::iterator found = mActors.find(ptr.mRef);
        if (found != mActors.end())
            return found->second.get();
        return nullptr;
    }

    const Actor* PhysicsSystem::getActor(const MWWorld::ConstPtr& ptr) const
    {
        ActorMap::const_iterator found = mActors.find(ptr.mRef);
        if (found != mActors.end())
            return found->second.get();
        return nullptr;
    }

    const Object* PhysicsSystem::getObject(const MWWorld::ConstPtr& ptr) const
    {
        ObjectMap::const_iterator found = mObjects.find(ptr.mRef);
        if (found != mObjects.end())
            return found->second.get();
        return nullptr;
    }

    Projectile* PhysicsSystem::getProjectile(int projectileId) const
    {
        ProjectileMap::const_iterator found = mProjectiles.find(projectileId);
        if (found != mProjectiles.end())
            return found->second.get();
        return nullptr;
    }

    void PhysicsSystem::updateScale(const MWWorld::Ptr& ptr)
    {
        if (auto foundObject = mObjects.find(ptr.mRef); foundObject != mObjects.end())
        {
            float scale = ptr.getCellRef().getScale();
            foundObject->second->setScale(scale);
            mTaskScheduler->updateSingleAabb(foundObject->second);
        }
        else if (auto foundActor = mActors.find(ptr.mRef); foundActor != mActors.end())
        {
            foundActor->second->updateScale();
            mTaskScheduler->updateSingleAabb(foundActor->second);
        }
    }

    void PhysicsSystem::updateRotation(const MWWorld::Ptr& ptr, osg::Quat rotate)
    {
        if (auto foundObject = mObjects.find(ptr.mRef); foundObject != mObjects.end())
        {
            foundObject->second->setRotation(rotate);
            mTaskScheduler->updateSingleAabb(foundObject->second);
        }
        else if (auto foundActor = mActors.find(ptr.mRef); foundActor != mActors.end())
        {
            if (!foundActor->second->isRotationallyInvariant())
            {
                foundActor->second->setRotation(rotate);
                mTaskScheduler->updateSingleAabb(foundActor->second);
            }
        }
    }

    void PhysicsSystem::updatePosition(const MWWorld::Ptr& ptr)
    {
        if (auto foundObject = mObjects.find(ptr.mRef); foundObject != mObjects.end())
        {
            foundObject->second->updatePosition();
            mTaskScheduler->updateSingleAabb(foundObject->second);
        }
        else if (auto foundActor = mActors.find(ptr.mRef); foundActor != mActors.end())
        {
            foundActor->second->updatePosition();
            mTaskScheduler->updateSingleAabb(foundActor->second, true);
        }
    }

    void PhysicsSystem::addActor(const MWWorld::Ptr& ptr, VFS::Path::NormalizedView mesh)
    {
        const VFS::Path::Normalized animationMesh
            = Misc::ResourceHelpers::correctActorModelPath(mesh, mResourceSystem->getVFS());
        osg::ref_ptr<const Resource::BulletShape> shape = mShapeManager->getShape(animationMesh);

        // Try to get shape from basic model as fallback for creatures
        if (!ptr.getClass().isNpc() && shape && shape->mCollisionBox.mExtents.length2() == 0)
        {
            if (animationMesh != mesh)
            {
                shape = mShapeManager->getShape(mesh);
            }
        }

        if (!shape)
            return;

        // check if Actor should spawn above water
        const MWMechanics::MagicEffects& effects = ptr.getClass().getCreatureStats(ptr).getMagicEffects();
        const bool canWaterWalk = effects.getOrDefault(ESM::MagicEffect::WaterWalking).getMagnitude() > 0;

        auto actor = std::make_shared<Actor>(
            ptr, shape, mTaskScheduler.get(), canWaterWalk, Settings::game().mActorCollisionShapeType);

        mActors.emplace(ptr.mRef, std::move(actor));
    }

    int PhysicsSystem::addProjectile(
        const MWWorld::Ptr& caster, const osg::Vec3f& position, VFS::Path::NormalizedView mesh, bool computeRadius)
    {
        osg::ref_ptr<Resource::BulletShapeInstance> shapeInstance = mShapeManager->getInstance(mesh);
        assert(shapeInstance);
        float radius = computeRadius ? shapeInstance->mCollisionBox.mExtents.length() / 2.f : 1.f;

        mProjectileId++;

        auto projectile = std::make_shared<Projectile>(caster, position, radius, mTaskScheduler.get(), this);
        mProjectiles.emplace(mProjectileId, std::move(projectile));

        return mProjectileId;
    }

    void PhysicsSystem::setCaster(int projectileId, const MWWorld::Ptr& caster)
    {
        const auto foundProjectile = mProjectiles.find(projectileId);
        assert(foundProjectile != mProjectiles.end());
        auto* projectile = foundProjectile->second.get();

        projectile->setCaster(caster);
    }

    bool PhysicsSystem::toggleCollisionMode()
    {
        ActorMap::iterator found = mActors.find(MWMechanics::getPlayer().mRef);
        if (found != mActors.end())
        {
            bool cmode = found->second->getCollisionMode();
            cmode = !cmode;
            found->second->enableCollisionMode(cmode);
            // NB: Collision body isn't disabled for vanilla TCL compatibility
            return cmode;
        }

        return false;
    }

    void PhysicsSystem::queueObjectMovement(const MWWorld::Ptr& ptr, const osg::Vec3f& velocity)
    {
        ActorMap::iterator found = mActors.find(ptr.mRef);
        if (found != mActors.end())
            found->second->setVelocity(velocity);
    }

    void PhysicsSystem::clearQueuedMovement()
    {
        for (const auto& [_, actor] : mActors)
        {
            actor->setVelocity(osg::Vec3f());
            actor->setInertialForce(osg::Vec3f());
        }
    }

    void PhysicsSystem::prepareSimulation(bool willSimulate, std::vector<Simulation>& simulations)
    {
        assert(simulations.empty());
        simulations.reserve(mActors.size() + mProjectiles.size());
        const MWBase::World* world = MWBase::Environment::get().getWorld();
        for (const auto& [ref, physicActor] : mActors)
        {
            if (!physicActor->isActive())
                continue;

            auto ptr = physicActor->getPtr();
            if (!ptr.getClass().isMobile(ptr))
                continue;

            const MWWorld::CellStore& cell = *ptr.getCell();
            const auto& stats = ptr.getClass().getCreatureStats(ptr);
            const MWMechanics::MagicEffects& effects = stats.getMagicEffects();

            float waterlevel = -std::numeric_limits<float>::max();
            bool waterCollision = false;
            if (cell.getCell()->hasWater())
            {
                waterlevel = cell.getWaterLevel();
                if (physicActor->getCollisionMode())
                    waterCollision = effects.getOrDefault(ESM::MagicEffect::WaterWalking).getMagnitude();
            }

            physicActor->setCanWaterWalk(waterCollision);

            // Slow fall reduces fall speed by a factor of (effect magnitude / 200)
            const float slowFall
                = 1.f - std::clamp(effects.getOrDefault(ESM::MagicEffect::SlowFall).getMagnitude() * 0.005f, 0.f, 1.f);
            const bool isPlayer = ptr == world->getPlayerConstPtr();
            const bool godmode = isPlayer && world->getGodModeState();
            const bool inert = stats.isDead()
                || (!godmode && stats.getMagicEffects().getOrDefault(ESM::MagicEffect::Paralyze).getModifier() > 0);

            simulations.emplace_back(ActorSimulation{
                physicActor, ActorFrameData{ *physicActor, inert, waterCollision, slowFall, waterlevel, isPlayer } });

            // if the simulation will run, a jump request will be fulfilled. Update mechanics accordingly.
            if (willSimulate)
                handleJump(ptr);
        }

        for (const auto& [id, projectile] : mProjectiles)
        {
            simulations.emplace_back(ProjectileSimulation{ projectile, ProjectileFrameData{ *projectile } });
        }
    }

    void PhysicsSystem::stepSimulation(
        float dt, bool skipSimulation, osg::Timer_t frameStart, unsigned int frameNumber, osg::Stats& stats)
    {
        for (auto& [animatedObject, changed] : mAnimatedObjects)
        {
            if (animatedObject->animateCollisionShapes())
            {
                auto obj = mObjects.find(animatedObject->getPtr().mRef);
                assert(obj != mObjects.end());
                mTaskScheduler->updateSingleAabb(obj->second);
                changed = true;
            }
            else
            {
                changed = false;
            }
        }
        for (auto& [_, object] : mObjects)
            object->resetCollisions();

#ifndef BT_NO_PROFILE
        CProfileManager::Reset();
        CProfileManager::Increment_Frame_Counter();
#endif

        mTimeAccum += dt;

        if (skipSimulation)
            mTaskScheduler->resetSimulation(mActors);
        else
        {
            std::vector<Simulation>& simulations = mSimulations[mSimulationsCounter++ % mSimulations.size()];
            prepareSimulation(mTimeAccum >= mPhysicsDt, simulations);
            // modifies mTimeAccum
            mTaskScheduler->applyQueuedMovements(mTimeAccum, simulations, frameStart, frameNumber, stats);
        }
    }

    void PhysicsSystem::moveActors()
    {
        auto* player = getActor(MWMechanics::getPlayer());
        const auto world = MWBase::Environment::get().getWorld();

        // copy new ptr position in temporary vector. player is handled separately as its movement might change active
        // cell.
        mActorsPositions.clear();
        if (!mActors.empty())
            mActorsPositions.reserve(mActors.size() - 1);
        for (const auto& [ptr, physicActor] : mActors)
        {
            if (physicActor.get() == player)
                continue;
            mActorsPositions.emplace_back(physicActor->getPtr(), physicActor->getSimulationPosition());
        }

        for (const auto& [ptr, pos] : mActorsPositions)
            world->moveObject(ptr, pos, false, false);

        if (player != nullptr)
            world->moveObject(player->getPtr(), player->getSimulationPosition(), false, false);
    }

    void PhysicsSystem::updateAnimatedCollisionShape(const MWWorld::Ptr& object)
    {
        ObjectMap::iterator found = mObjects.find(object.mRef);
        if (found != mObjects.end())
            if (found->second->animateCollisionShapes())
                mTaskScheduler->updateSingleAabb(found->second);
    }

    void PhysicsSystem::debugDraw()
    {
        if (mDebugDrawEnabled)
            mTaskScheduler->debugDraw();
    }

    bool PhysicsSystem::isActorStandingOn(const MWWorld::Ptr& actor, const MWWorld::ConstPtr& object) const
    {
        const auto physActor = mActors.find(actor.mRef);
        if (physActor != mActors.end())
            return physActor->second->getStandingOnPtr() == object;
        return false;
    }

    void PhysicsSystem::getActorsStandingOn(const MWWorld::ConstPtr& object, std::vector<MWWorld::Ptr>& out) const
    {
        for (const auto& [_, actor] : mActors)
        {
            if (actor->getStandingOnPtr() == object)
                out.emplace_back(actor->getPtr());
        }
    }

    bool PhysicsSystem::isObjectCollidingWith(const MWWorld::ConstPtr& object, ScriptedCollisionType type) const
    {
        auto found = mObjects.find(object.mRef);
        if (found != mObjects.end())
            return found->second->collidedWith(type);
        return false;
    }

    void PhysicsSystem::getActorsCollidingWith(const MWWorld::ConstPtr& object, std::vector<MWWorld::Ptr>& out) const
    {
        std::vector<MWWorld::Ptr> collisions = getCollisions(object, CollisionType_World, CollisionType_Actor);
        out.insert(out.end(), collisions.begin(), collisions.end());
    }

    void PhysicsSystem::disableWater()
    {
        if (mWaterEnabled)
        {
            mWaterEnabled = false;
            updateWater();
        }
    }

    void PhysicsSystem::enableWater(float height)
    {
        if (!mWaterEnabled || mWaterHeight != height)
        {
            mWaterEnabled = true;
            mWaterHeight = height;
            updateWater();
        }
    }

    void PhysicsSystem::setWaterHeight(float height)
    {
        if (mWaterHeight != height)
        {
            mWaterHeight = height;
            updateWater();
        }
    }

    void PhysicsSystem::updateWater()
    {
        if (mWaterCollisionObject)
        {
            mTaskScheduler->removeCollisionObject(mWaterCollisionObject.get());
        }

        if (!mWaterEnabled)
        {
            mWaterCollisionObject.reset();
            return;
        }

        mWaterCollisionObject = std::make_unique<btCollisionObject>();
        mWaterCollisionShape = std::make_unique<btStaticPlaneShape>(btVector3(0, 0, 1), mWaterHeight);
        mWaterCollisionObject->setCollisionShape(mWaterCollisionShape.get());
        mTaskScheduler->addCollisionObject(
            mWaterCollisionObject.get(), CollisionType_Water, CollisionType_Actor | CollisionType_Projectile);
    }

    bool PhysicsSystem::isAreaOccupiedByOtherActor(
        const MWWorld::LiveCellRefBase* actor, const osg::Vec3f& position, const float radius) const
    {
        const btCollisionObject* ignoredObject = nullptr;
        if (const auto it = mActors.find(actor); it != mActors.end())
            ignoredObject = it->second->getCollisionObject();
        const btVector3 bulletPosition = Misc::Convert::toBullet(position);
        const btVector3 aabbMin = bulletPosition - btVector3(radius, radius, radius);
        const btVector3 aabbMax = bulletPosition + btVector3(radius, radius, radius);
        const int mask = MWPhysics::CollisionType_Actor;
        const int group = MWPhysics::CollisionType_AnyPhysical;
        HasSphereCollisionCallback callback(bulletPosition, radius, mask, group, ignoredObject);
        mTaskScheduler->aabbTest(aabbMin, aabbMax, callback);
        return callback.getResult();
    }

    void PhysicsSystem::reportStats(unsigned int frameNumber, osg::Stats& stats) const
    {
        stats.setAttribute(frameNumber, "Physics Actors", static_cast<double>(mActors.size()));
        stats.setAttribute(frameNumber, "Physics Objects", static_cast<double>(mObjects.size()));
        stats.setAttribute(frameNumber, "Physics Projectiles", static_cast<double>(mProjectiles.size()));
        stats.setAttribute(frameNumber, "Physics HeightFields", static_cast<double>(mHeightFields.size()));
    }

    void PhysicsSystem::reportCollision(const btVector3& position, const btVector3& normal)
    {
        if (mDebugDrawEnabled)
            mDebugDrawer->addCollision(position, normal);
    }

    ActorFrameData::ActorFrameData(
        Actor& actor, bool inert, bool waterCollision, float slowFall, float waterlevel, bool isPlayer)
        : mPosition()
        , mStandingOn(nullptr)
        , mIsOnGround(actor.getOnGround())
        , mIsOnSlope(actor.getOnSlope())
        , mWalkingOnWater(false)
        , mInert(inert)
        , mCollisionObject(actor.getCollisionObject())
        , mSwimLevel(waterlevel
              - (actor.getRenderingHalfExtents().z() * 2
                  * MWBase::Environment::get()
                        .getESMStore()
                        ->get<ESM::GameSetting>()
                        .find("fSwimHeightScale")
                        ->mValue.getFloat()))
        , mSlowFall(slowFall)
        , mRotation()
        , mMovement(actor.velocity())
        , mWaterlevel(waterlevel)
        , mHalfExtentsZ(actor.getHalfExtents().z())
        , mOldHeight(0)
        , mStuckFrames(0)
        , mFlying(MWBase::Environment::get().getWorld()->isFlying(actor.getPtr()))
        , mWasOnGround(actor.getOnGround())
        , mIsAquatic(actor.getPtr().getClass().isPureWaterCreature(actor.getPtr()))
        , mWaterCollision(waterCollision)
        , mSkipCollisionDetection(!actor.getCollisionMode())
        , mIsPlayer(isPlayer)
    {
    }

    ProjectileFrameData::ProjectileFrameData(Projectile& projectile)
        : mPosition(projectile.getPosition())
        , mMovement(projectile.velocity())
        , mCaster(projectile.getCasterCollisionObject())
        , mCollisionObject(projectile.getCollisionObject())
        , mProjectile(&projectile)
    {
    }

    WorldFrameData::WorldFrameData()
        : mIsInStorm(MWBase::Environment::get().getWorld()->isInStorm())
        , mStormDirection(MWBase::Environment::get().getWorld()->getStormDirection())
    {
    }

    LOSRequest::LOSRequest(const std::weak_ptr<Actor>& a1, const std::weak_ptr<Actor>& a2)
        : mResult(false)
        , mStale(false)
        , mAge(0)
    {
        // we use raw actor pointer pair to uniquely identify request
        // sort the pointer value in ascending order to not duplicate equivalent requests, eg. getLOS(A, B) and
        // getLOS(B, A)
        auto* raw1 = a1.lock().get();
        auto* raw2 = a2.lock().get();
        assert(raw1 != raw2);
        if (raw1 < raw2)
        {
            mActors = { a1, a2 };
            mRawActors = { raw1, raw2 };
        }
        else
        {
            mActors = { a2, a1 };
            mRawActors = { raw2, raw1 };
        }
    }

    bool operator==(const LOSRequest& lhs, const LOSRequest& rhs) noexcept
    {
        return lhs.mRawActors == rhs.mRawActors;
    }
}
