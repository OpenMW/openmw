#include "physicssystem.hpp"

#include <LinearMath/btIDebugDraw.h>
#include <LinearMath/btVector3.h>
#include <memory>
#include <osg/Group>
#include <osg/Stats>
#include <osg/Timer>

#include <BulletCollision/CollisionShapes/btConeShape.h>
#include <BulletCollision/CollisionShapes/btSphereShape.h>
#include <BulletCollision/CollisionShapes/btStaticPlaneShape.h>
#include <BulletCollision/CollisionShapes/btCompoundShape.h>
#include <BulletCollision/CollisionDispatch/btCollisionObject.h>
#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>
#include <BulletCollision/CollisionDispatch/btDefaultCollisionConfiguration.h>
#include <BulletCollision/BroadphaseCollision/btDbvtBroadphase.h>

#include <LinearMath/btQuickprof.h>

#include <components/nifbullet/bulletnifloader.hpp>
#include <components/resource/resourcesystem.hpp>
#include <components/resource/bulletshapemanager.hpp>
#include <components/debug/debuglog.hpp>
#include <components/esm/loadgmst.hpp>
#include <components/sceneutil/positionattitudetransform.hpp>
#include <components/sceneutil/unrefqueue.hpp>
#include <components/misc/convert.hpp>

#include <components/nifosg/particle.hpp> // FindRecIndexVisitor

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/movement.hpp"

#include "../mwworld/esmstore.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/player.hpp"

#include "../mwrender/bulletdebugdraw.hpp"

#include "../mwworld/class.hpp"

#include "collisiontype.hpp"
#include "actor.hpp"

#include "projectile.hpp"
#include "trace.h"
#include "object.hpp"
#include "heightfield.hpp"
#include "hasspherecollisioncallback.hpp"
#include "deepestnotmecontacttestresultcallback.hpp"
#include "closestnotmerayresultcallback.hpp"
#include "contacttestresultcallback.hpp"
#include "projectileconvexcallback.hpp"
#include "movementsolver.hpp"
#include "mtphysics.hpp"

namespace
{
    bool canMoveToWaterSurface(const MWPhysics::Actor* physicActor, const float waterlevel, btCollisionWorld* world)
    {
        if (!physicActor)
            return false;
        const float halfZ = physicActor->getHalfExtents().z();
        const osg::Vec3f actorPosition = physicActor->getPosition();
        const osg::Vec3f startingPosition(actorPosition.x(), actorPosition.y(), actorPosition.z() + halfZ);
        const osg::Vec3f destinationPosition(actorPosition.x(), actorPosition.y(), waterlevel + halfZ);
        MWPhysics::ActorTracer tracer;
        tracer.doTrace(physicActor->getCollisionObject(), startingPosition, destinationPosition, world);
        return (tracer.mFraction >= 1.0f);
    }

    void handleJump(const MWWorld::Ptr &ptr)
    {
        if (!ptr.getClass().isActor())
            return;
        if (ptr.getClass().getMovementSettings(ptr).mPosition[2] == 0)
            return;
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

}

namespace MWPhysics
{
    PhysicsSystem::PhysicsSystem(Resource::ResourceSystem* resourceSystem, osg::ref_ptr<osg::Group> parentNode)
        : mShapeManager(new Resource::BulletShapeManager(resourceSystem->getVFS(), resourceSystem->getSceneManager(), resourceSystem->getNifFileManager()))
        , mResourceSystem(resourceSystem)
        , mDebugDrawEnabled(false)
        , mTimeAccum(0.0f)
        , mProjectileId(0)
        , mWaterHeight(0)
        , mWaterEnabled(false)
        , mParentNode(parentNode)
        , mPhysicsDt(1.f / 60.f)
    {
        mResourceSystem->addResourceManager(mShapeManager.get());

        mCollisionConfiguration = std::make_unique<btDefaultCollisionConfiguration>();
        mDispatcher = std::make_unique<btCollisionDispatcher>(mCollisionConfiguration.get());
        mBroadphase = std::make_unique<btDbvtBroadphase>();

        mCollisionWorld = std::make_unique<btCollisionWorld>(mDispatcher.get(), mBroadphase.get(), mCollisionConfiguration.get());

        // Don't update AABBs of all objects every frame. Most objects in MW are static, so we don't need this.
        // Should a "static" object ever be moved, we have to update its AABB manually using DynamicsWorld::updateSingleAabb.
        mCollisionWorld->setForceUpdateAllAabbs(false);

        // Check if a user decided to override a physics system FPS
        const char* env = getenv("OPENMW_PHYSICS_FPS");
        if (env)
        {
            float physFramerate = std::atof(env);
            if (physFramerate > 0)
            {
                mPhysicsDt = 1.f / physFramerate;
                Log(Debug::Warning) << "Warning: using custom physics framerate (" << physFramerate << " FPS).";
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

    void PhysicsSystem::setUnrefQueue(SceneUtil::UnrefQueue *unrefQueue)
    {
        mUnrefQueue = unrefQueue;
    }

    Resource::BulletShapeManager *PhysicsSystem::getShapeManager()
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

    void PhysicsSystem::markAsNonSolid(const MWWorld::ConstPtr &ptr)
    {
        ObjectMap::iterator found = mObjects.find(ptr.mRef);
        if (found == mObjects.end())
            return;

        found->second->setSolid(false);
    }

    bool PhysicsSystem::isOnSolidGround (const MWWorld::Ptr& actor) const
    {
        const Actor* physactor = getActor(actor);
        if (!physactor || !physactor->getOnGround())
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

    std::pair<MWWorld::Ptr, osg::Vec3f> PhysicsSystem::getHitContact(const MWWorld::ConstPtr& actor,
                                                                     const osg::Vec3f &origin,
                                                                     const osg::Quat &orient,
                                                                     float queryDistance, std::vector<MWWorld::Ptr>& targets)
    {
        // First of all, try to hit where you aim to
        int hitmask = CollisionType_World | CollisionType_Door | CollisionType_HeightMap | CollisionType_Actor;
        RayCastingResult result = castRay(origin, origin + (orient * osg::Vec3f(0.0f, queryDistance, 0.0f)), actor, targets, hitmask, CollisionType_Actor);

        if (result.mHit)
        {
            reportCollision(Misc::Convert::toBullet(result.mHitPos), Misc::Convert::toBullet(result.mHitNormal));
            return std::make_pair(result.mHitObject, result.mHitPos);
        }

        // Use cone shape as fallback
        const MWWorld::Store<ESM::GameSetting> &store = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

        btConeShape shape (osg::DegreesToRadians(store.find("fCombatAngleXY")->mValue.getFloat()/2.0f), queryDistance);
        shape.setLocalScaling(btVector3(1, 1, osg::DegreesToRadians(store.find("fCombatAngleZ")->mValue.getFloat()/2.0f) /
                                              shape.getRadius()));

        // The shape origin is its center, so we have to move it forward by half the length. The
        // real origin will be provided to getFilteredContact to find the closest.
        osg::Vec3f center = origin + (orient * osg::Vec3f(0.0f, queryDistance*0.5f, 0.0f));

        btCollisionObject object;
        object.setCollisionShape(&shape);
        object.setWorldTransform(btTransform(Misc::Convert::toBullet(orient), Misc::Convert::toBullet(center)));

        const btCollisionObject* me = nullptr;
        std::vector<const btCollisionObject*> targetCollisionObjects;

        const Actor* physactor = getActor(actor);
        if (physactor)
            me = physactor->getCollisionObject();

        if (!targets.empty())
        {
            for (MWWorld::Ptr& target : targets)
            {
                const Actor* targetActor = getActor(target);
                if (targetActor)
                    targetCollisionObjects.push_back(targetActor->getCollisionObject());
            }
        }

        DeepestNotMeContactTestResultCallback resultCallback(me, targetCollisionObjects, Misc::Convert::toBullet(origin));
        resultCallback.m_collisionFilterGroup = CollisionType_Actor;
        resultCallback.m_collisionFilterMask = CollisionType_World | CollisionType_Door | CollisionType_HeightMap | CollisionType_Actor;
        mTaskScheduler->contactTest(&object, resultCallback);

        if (resultCallback.mObject)
        {
            PtrHolder* holder = static_cast<PtrHolder*>(resultCallback.mObject->getUserPointer());
            if (holder)
            {
                reportCollision(resultCallback.mContactPoint, resultCallback.mContactNormal);
                return std::make_pair(holder->getPtr(), Misc::Convert::toOsg(resultCallback.mContactPoint));
            }
        }
        return std::make_pair(MWWorld::Ptr(), osg::Vec3f());
    }

    float PhysicsSystem::getHitDistance(const osg::Vec3f &point, const MWWorld::ConstPtr &target) const
    {
        btCollisionObject* targetCollisionObj = nullptr;
        const Actor* actor = getActor(target);
        if (actor)
            targetCollisionObj = actor->getCollisionObject();
        if (!targetCollisionObj)
            return 0.f;

        btTransform rayFrom;
        rayFrom.setIdentity();
        rayFrom.setOrigin(Misc::Convert::toBullet(point));

        auto hitpoint = mTaskScheduler->getHitPoint(rayFrom, targetCollisionObj);
        if (hitpoint)
            return (point - Misc::Convert::toOsg(*hitpoint)).length();

        // didn't hit the target. this could happen if point is already inside the collision box
        return 0.f;
    }

    RayCastingResult PhysicsSystem::castRay(const osg::Vec3f &from, const osg::Vec3f &to, const MWWorld::ConstPtr& ignore, const std::vector<MWWorld::Ptr>& targets, int mask, int group) const
    {
        if (from == to)
        {
            RayCastingResult result;
            result.mHit = false;
            return result;
        }
        btVector3 btFrom = Misc::Convert::toBullet(from);
        btVector3 btTo = Misc::Convert::toBullet(to);

        const btCollisionObject* me = nullptr;
        std::vector<const btCollisionObject*> targetCollisionObjects;

        if (!ignore.isEmpty())
        {
            const Actor* actor = getActor(ignore);
            if (actor)
                me = actor->getCollisionObject();
            else
            {
                const Object* object = getObject(ignore);
                if (object)
                    me = object->getCollisionObject();
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

        ClosestNotMeRayResultCallback resultCallback(me, targetCollisionObjects, btFrom, btTo);
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

    RayCastingResult PhysicsSystem::castSphere(const osg::Vec3f &from, const osg::Vec3f &to, float radius, int mask, int group) const
    {
        btCollisionWorld::ClosestConvexResultCallback callback(Misc::Convert::toBullet(from), Misc::Convert::toBullet(to));
        callback.m_collisionFilterGroup = group;
        callback.m_collisionFilterMask = mask;

        btSphereShape shape(radius);
        const btQuaternion btrot = btQuaternion::getIdentity();

        btTransform from_ (btrot, Misc::Convert::toBullet(from));
        btTransform to_ (btrot, Misc::Convert::toBullet(to));

        mTaskScheduler->convexSweepTest(&shape, from_, to_, callback);

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

    bool PhysicsSystem::getLineOfSight(const MWWorld::ConstPtr &actor1, const MWWorld::ConstPtr &actor2) const
    {
        const auto it1 = mActors.find(actor1.mRef);
        const auto it2 = mActors.find(actor2.mRef);
        if (it1 == mActors.end() || it2 == mActors.end())
            return false;

        return mTaskScheduler->getLineOfSight(it1->second, it2->second);
    }

    bool PhysicsSystem::isOnGround(const MWWorld::Ptr &actor)
    {
        Actor* physactor = getActor(actor);
        return physactor && physactor->getOnGround();
    }

    bool PhysicsSystem::canMoveToWaterSurface(const MWWorld::ConstPtr &actor, const float waterlevel)
    {
        return ::canMoveToWaterSurface(getActor(actor), waterlevel, mCollisionWorld.get());
    }

    osg::Vec3f PhysicsSystem::getHalfExtents(const MWWorld::ConstPtr &actor) const
    {
        const Actor* physactor = getActor(actor);
        if (physactor)
            return physactor->getHalfExtents();
        else
            return osg::Vec3f();
    }

    osg::Vec3f PhysicsSystem::getOriginalHalfExtents(const MWWorld::ConstPtr &actor) const
    {
        if (const Actor* physactor = getActor(actor))
            return physactor->getOriginalHalfExtents();
        else
            return osg::Vec3f();
    }

    osg::Vec3f PhysicsSystem::getRenderingHalfExtents(const MWWorld::ConstPtr &actor) const
    {
        const Actor* physactor = getActor(actor);
        if (physactor)
            return physactor->getRenderingHalfExtents();
        else
            return osg::Vec3f();
    }

    osg::BoundingBox PhysicsSystem::getBoundingBox(const MWWorld::ConstPtr &object) const
    {
        const Object * physobject = getObject(object);
        if (!physobject) return osg::BoundingBox();
        btVector3 min, max;
        mTaskScheduler->getAabb(physobject->getCollisionObject(), min, max);
        return osg::BoundingBox(Misc::Convert::toOsg(min), Misc::Convert::toOsg(max));
    }

    osg::Vec3f PhysicsSystem::getCollisionObjectPosition(const MWWorld::ConstPtr &actor) const
    {
        const Actor* physactor = getActor(actor);
        if (physactor)
            return physactor->getCollisionObjectPosition();
        else
            return osg::Vec3f();
    }

    std::vector<ContactPoint> PhysicsSystem::getCollisionsPoints(const MWWorld::ConstPtr &ptr, int collisionGroup, int collisionMask) const
    {
        btCollisionObject* me = nullptr;

        auto found = mObjects.find(ptr.mRef);
        if (found != mObjects.end())
            me = found->second->getCollisionObject();
        else
            return {};

        ContactTestResultCallback resultCallback (me);
        resultCallback.m_collisionFilterGroup = collisionGroup;
        resultCallback.m_collisionFilterMask = collisionMask;
        mTaskScheduler->contactTest(me, resultCallback);
        return resultCallback.mResult;
    }

    std::vector<MWWorld::Ptr> PhysicsSystem::getCollisions(const MWWorld::ConstPtr &ptr, int collisionGroup, int collisionMask) const
    {
        std::vector<MWWorld::Ptr> actors;
        for (auto& [actor, point, normal] : getCollisionsPoints(ptr, collisionGroup, collisionMask))
            actors.emplace_back(actor);
        return actors;
    }

    osg::Vec3f PhysicsSystem::traceDown(const MWWorld::Ptr &ptr, const osg::Vec3f& position, float maxHeight)
    {
        ActorMap::iterator found = mActors.find(ptr.mRef);
        if (found ==  mActors.end())
            return ptr.getRefData().getPosition().asVec3();
        return MovementSolver::traceDown(ptr, position, found->second.get(), mCollisionWorld.get(), maxHeight);
    }

    void PhysicsSystem::addHeightField (const float* heights, int x, int y, float triSize, float sqrtVerts, float minH, float maxH, const osg::Object* holdObject)
    {
        mHeightFields[std::make_pair(x,y)] = std::make_unique<HeightField>(heights, x, y, triSize, sqrtVerts, minH, maxH, holdObject, mTaskScheduler.get());
    }

    void PhysicsSystem::removeHeightField (int x, int y)
    {
        HeightFieldMap::iterator heightfield = mHeightFields.find(std::make_pair(x,y));
        if(heightfield != mHeightFields.end())
            mHeightFields.erase(heightfield);
    }

    const HeightField* PhysicsSystem::getHeightField(int x, int y) const
    {
        const auto heightField = mHeightFields.find(std::make_pair(x, y));
        if (heightField == mHeightFields.end())
            return nullptr;
        return heightField->second.get();
    }

    void PhysicsSystem::addObject (const MWWorld::Ptr& ptr, const std::string& mesh, osg::Quat rotation, int collisionType)
    {
        if (ptr.mRef->mData.mPhysicsPostponed)
            return;
        osg::ref_ptr<Resource::BulletShapeInstance> shapeInstance = mShapeManager->getInstance(mesh);
        if (!shapeInstance || !shapeInstance->getCollisionShape())
            return;

        assert(!getObject(ptr));

        auto obj = std::make_shared<Object>(ptr, shapeInstance, rotation, collisionType, mTaskScheduler.get());
        mObjects.emplace(ptr.mRef, obj);

        if (obj->isAnimated())
            mAnimatedObjects.insert(obj.get());
    }

    void PhysicsSystem::remove(const MWWorld::Ptr &ptr)
    {
        if (auto foundObject = mObjects.find(ptr.mRef); foundObject != mObjects.end())
        {
            if (mUnrefQueue.get())
                mUnrefQueue->push(foundObject->second->getShapeInstance());

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

    void PhysicsSystem::updatePtr(const MWWorld::Ptr &old, const MWWorld::Ptr &updated)
    {
        if (auto found = mObjects.find(old.mRef); found != mObjects.end())
            found->second->updatePtr(updated);
        else if (auto found = mActors.find(old.mRef); found != mActors.end())
            found->second->updatePtr(updated);

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

    Actor *PhysicsSystem::getActor(const MWWorld::Ptr &ptr)
    {
        ActorMap::iterator found = mActors.find(ptr.mRef);
        if (found != mActors.end())
            return found->second.get();
        return nullptr;
    }

    const Actor *PhysicsSystem::getActor(const MWWorld::ConstPtr &ptr) const
    {
        ActorMap::const_iterator found = mActors.find(ptr.mRef);
        if (found != mActors.end())
            return found->second.get();
        return nullptr;
    }

    const Object* PhysicsSystem::getObject(const MWWorld::ConstPtr &ptr) const
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

    void PhysicsSystem::updateScale(const MWWorld::Ptr &ptr)
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

    void PhysicsSystem::updateProjectile(const int projectileId, const osg::Vec3f &position) const
    {
        const auto foundProjectile = mProjectiles.find(projectileId);
        assert(foundProjectile != mProjectiles.end());
        auto* projectile = foundProjectile->second.get();

        btVector3 btFrom = Misc::Convert::toBullet(projectile->getPosition());
        btVector3 btTo = Misc::Convert::toBullet(position);

        if (btFrom == btTo)
            return;

        ProjectileConvexCallback resultCallback(projectile->getCasterCollisionObject(), projectile->getCollisionObject(), btFrom, btTo, projectile);
        resultCallback.m_collisionFilterMask = 0xff;
        resultCallback.m_collisionFilterGroup = CollisionType_Projectile;

        const btQuaternion btrot = btQuaternion::getIdentity();
        btTransform from_ (btrot, btFrom);
        btTransform to_ (btrot, btTo);

        mTaskScheduler->convexSweepTest(projectile->getConvexShape(), from_, to_, resultCallback);

        const auto newpos = projectile->isActive() ? position : Misc::Convert::toOsg(projectile->getHitPosition());
        projectile->setPosition(newpos);
        mTaskScheduler->updateSingleAabb(foundProjectile->second);
    }

    void PhysicsSystem::updateRotation(const MWWorld::Ptr &ptr, osg::Quat rotate)
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

    void PhysicsSystem::updatePosition(const MWWorld::Ptr &ptr)
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

    void PhysicsSystem::addActor (const MWWorld::Ptr& ptr, const std::string& mesh)
    {
        osg::ref_ptr<const Resource::BulletShape> shape = mShapeManager->getShape(mesh);

        // Try to get shape from basic model as fallback for creatures
        if (!ptr.getClass().isNpc() && shape && shape->mCollisionBox.extents.length2() == 0)
        {
            const std::string fallbackModel = ptr.getClass().getModel(ptr);
            if (fallbackModel != mesh)
            {
                shape = mShapeManager->getShape(fallbackModel);
            }
        }

        if (!shape)
            return;

        // check if Actor should spawn above water
        const MWMechanics::MagicEffects& effects = ptr.getClass().getCreatureStats(ptr).getMagicEffects();
        const bool canWaterWalk = effects.get(ESM::MagicEffect::WaterWalking).getMagnitude() > 0;

        auto actor = std::make_shared<Actor>(ptr, shape, mTaskScheduler.get(), canWaterWalk);
        
        mActors.emplace(ptr.mRef, std::move(actor));
    }

    int PhysicsSystem::addProjectile (const MWWorld::Ptr& caster, const osg::Vec3f& position, const std::string& mesh, bool computeRadius)
    {
        osg::ref_ptr<Resource::BulletShapeInstance> shapeInstance = mShapeManager->getInstance(mesh);
        assert(shapeInstance);
        float radius = computeRadius ? shapeInstance->mCollisionBox.extents.length() / 2.f : 1.f;

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

    void PhysicsSystem::queueObjectMovement(const MWWorld::Ptr &ptr, const osg::Vec3f &velocity)
    {
        ActorMap::iterator found = mActors.find(ptr.mRef);
        if (found != mActors.end())
            found->second->setVelocity(velocity);
    }

    void PhysicsSystem::clearQueuedMovement()
    {
        for (const auto& [_, actor] : mActors)
            actor->setVelocity(osg::Vec3f());
    }

    std::pair<std::vector<std::shared_ptr<Actor>>, std::vector<ActorFrameData>> PhysicsSystem::prepareFrameData(bool willSimulate)
    {
        std::pair<std::vector<std::shared_ptr<Actor>>, std::vector<ActorFrameData>> framedata;
        framedata.first.reserve(mActors.size());
        framedata.second.reserve(mActors.size());
        const MWBase::World *world = MWBase::Environment::get().getWorld();
        for (const auto& [ref, physicActor] : mActors)
        {
            auto ptr = physicActor->getPtr();
            if (!ptr.getClass().isMobile(ptr))
                continue;
            float waterlevel = -std::numeric_limits<float>::max();
            const MWWorld::CellStore *cell = ptr.getCell();
            if(cell->getCell()->hasWater())
                waterlevel = cell->getWaterLevel();

            const auto& stats = ptr.getClass().getCreatureStats(ptr);
            const MWMechanics::MagicEffects& effects = stats.getMagicEffects();

            bool waterCollision = false;
            if (cell->getCell()->hasWater() && effects.get(ESM::MagicEffect::WaterWalking).getMagnitude())
            {
                if (physicActor->getCollisionMode() || !world->isUnderwater(ptr.getCell(), ptr.getRefData().getPosition().asVec3()))
                    waterCollision = true;
            }

            physicActor->setCanWaterWalk(waterCollision);

            // Slow fall reduces fall speed by a factor of (effect magnitude / 200)
            const float slowFall = 1.f - std::max(0.f, std::min(1.f, effects.get(ESM::MagicEffect::SlowFall).getMagnitude() * 0.005f));
            const bool godmode = ptr == world->getPlayerConstPtr() && world->getGodModeState();
            const bool inert = stats.isDead() || (!godmode && stats.getMagicEffects().get(ESM::MagicEffect::Paralyze).getModifier() > 0);

            framedata.first.emplace_back(physicActor);
            framedata.second.emplace_back(*physicActor, inert, waterCollision, slowFall, waterlevel);

            // if the simulation will run, a jump request will be fulfilled. Update mechanics accordingly.
            if (willSimulate)
                handleJump(ptr);
        }
        return framedata;
    }

    void PhysicsSystem::stepSimulation(float dt, bool skipSimulation, osg::Timer_t frameStart, unsigned int frameNumber, osg::Stats& stats)
    {
        for (Object* animatedObject : mAnimatedObjects)
        {
            if (animatedObject->animateCollisionShapes())
            {
                auto obj = mObjects.find(animatedObject->getPtr().mRef);
                assert(obj != mObjects.end());
                mTaskScheduler->updateSingleAabb(obj->second);
            }
        }

#ifndef BT_NO_PROFILE
        CProfileManager::Reset();
        CProfileManager::Increment_Frame_Counter();
#endif

        mTimeAccum += dt;

        if (skipSimulation)
            mTaskScheduler->resetSimulation(mActors);
        else
        {
            auto [actors, framedata] = prepareFrameData(mTimeAccum >= mPhysicsDt);
            // modifies mTimeAccum
            mTaskScheduler->applyQueuedMovements(mTimeAccum, std::move(actors), std::move(framedata), frameStart, frameNumber, stats);
        }
    }

    void PhysicsSystem::moveActors()
    {
        auto* player = getActor(MWMechanics::getPlayer());
        auto* world = MWBase::Environment::get().getWorld();

        // copy new ptr position in temporary vector. player is handled separately as its movement might change active cell.
        std::vector<std::pair<MWWorld::Ptr, osg::Vec3f>> newPositions;
        newPositions.reserve(mActors.size() - 1);
        for (const auto& [ptr, physicActor] : mActors)
        {
            if (physicActor.get() == player)
                continue;
            newPositions.emplace_back(physicActor->getPtr(), physicActor->getSimulationPosition());
        }

        for (auto& [ptr, pos] : newPositions)
            world->moveObject(ptr, pos, false, false);

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

    bool PhysicsSystem::isActorStandingOn(const MWWorld::Ptr &actor, const MWWorld::ConstPtr &object) const
    {
        const auto physActor = mActors.find(actor.mRef);
        if (physActor != mActors.end())
            return physActor->second->getStandingOnPtr() == object;
        return false;
    }

    void PhysicsSystem::getActorsStandingOn(const MWWorld::ConstPtr &object, std::vector<MWWorld::Ptr> &out) const
    {
        for (const auto& [_, actor] : mActors)
        {
            if (actor->getStandingOnPtr() == object)
                out.emplace_back(actor->getPtr());
        }
    }

    bool PhysicsSystem::isActorCollidingWith(const MWWorld::Ptr &actor, const MWWorld::ConstPtr &object) const
    {
        std::vector<MWWorld::Ptr> collisions = getCollisions(object, CollisionType_World, CollisionType_Actor);
        return (std::find(collisions.begin(), collisions.end(), actor) != collisions.end());
    }

    void PhysicsSystem::getActorsCollidingWith(const MWWorld::ConstPtr &object, std::vector<MWWorld::Ptr> &out) const
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

        mWaterCollisionObject.reset(new btCollisionObject());
        mWaterCollisionShape.reset(new btStaticPlaneShape(btVector3(0,0,1), mWaterHeight));
        mWaterCollisionObject->setCollisionShape(mWaterCollisionShape.get());
        mTaskScheduler->addCollisionObject(mWaterCollisionObject.get(), CollisionType_Water,
                                                    CollisionType_Actor|CollisionType_Projectile);
    }

    bool PhysicsSystem::isAreaOccupiedByOtherActor(const osg::Vec3f& position, const float radius, const MWWorld::ConstPtr& ignore) const
    {
        btCollisionObject* object = nullptr;
        const auto it = mActors.find(ignore.mRef);
        if (it != mActors.end())
            object = it->second->getCollisionObject();
        const auto bulletPosition = Misc::Convert::toBullet(position);
        const auto aabbMin = bulletPosition - btVector3(radius, radius, radius);
        const auto aabbMax = bulletPosition + btVector3(radius, radius, radius);
        const int mask = MWPhysics::CollisionType_Actor;
        const int group = 0xff;
        HasSphereCollisionCallback callback(bulletPosition, radius, object, mask, group);
        mTaskScheduler->aabbTest(aabbMin, aabbMax, callback);
        return callback.getResult();
    }

    void PhysicsSystem::reportStats(unsigned int frameNumber, osg::Stats& stats) const
    {
        stats.setAttribute(frameNumber, "Physics Actors", mActors.size());
        stats.setAttribute(frameNumber, "Physics Objects", mObjects.size());
        stats.setAttribute(frameNumber, "Physics Projectiles", mProjectiles.size());
        stats.setAttribute(frameNumber, "Physics HeightFields", mHeightFields.size());
    }

    void PhysicsSystem::reportCollision(const btVector3& position, const btVector3& normal)
    {
        if (mDebugDrawEnabled)
            mDebugDrawer->addCollision(position, normal);
    }

    ActorFrameData::ActorFrameData(Actor& actor, bool inert, bool waterCollision, float slowFall, float waterlevel)
        : mPosition()
        , mStandingOn(nullptr)
        , mIsOnGround(actor.getOnGround())
        , mIsOnSlope(actor.getOnSlope())
        , mWalkingOnWater(false)
        , mInert(inert)
        , mCollisionObject(actor.getCollisionObject())
        , mSwimLevel(waterlevel - (actor.getRenderingHalfExtents().z() * 2 * MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fSwimHeightScale")->mValue.getFloat()))
        , mSlowFall(slowFall)
        , mRotation()
        , mMovement(actor.velocity())
        , mWaterlevel(waterlevel)
        , mHalfExtentsZ(actor.getHalfExtents().z())
        , mOldHeight(0)
        , mFallHeight(0)
        , mStuckFrames(0)
        , mFlying(MWBase::Environment::get().getWorld()->isFlying(actor.getPtr()))
        , mWasOnGround(actor.getOnGround())
        , mIsAquatic(actor.getPtr().getClass().isPureWaterCreature(actor.getPtr()))
        , mWaterCollision(waterCollision)
        , mSkipCollisionDetection(actor.skipCollisions() || !actor.getCollisionMode())
        , mNeedLand(false)
    {
    }

    void ActorFrameData::updatePosition(Actor& actor, btCollisionWorld* world)
    {
        actor.applyOffsetChange();
        mPosition = actor.getPosition();
        if (mWaterCollision && mPosition.z() < mWaterlevel && canMoveToWaterSurface(&actor, mWaterlevel, world))
        {
            mPosition.z() = mWaterlevel;
            MWBase::Environment::get().getWorld()->moveObject(actor.getPtr(), mPosition, false);
        }
        mOldHeight = mPosition.z();
        const auto rotation = actor.getPtr().getRefData().getPosition().asRotationVec3();
        mRotation = osg::Vec2f(rotation.x(), rotation.z());
        mInertia = actor.getInertialForce();
        mStuckFrames = actor.getStuckFrames();
        mLastStuckPosition = actor.getLastStuckPosition();
    }

    WorldFrameData::WorldFrameData()
        : mIsInStorm(MWBase::Environment::get().getWorld()->isInStorm())
        , mStormDirection(MWBase::Environment::get().getWorld()->getStormDirection())
    {}

    LOSRequest::LOSRequest(const std::weak_ptr<Actor>& a1, const std::weak_ptr<Actor>& a2)
        : mResult(false), mStale(false), mAge(0)
    {
        // we use raw actor pointer pair to uniquely identify request
        // sort the pointer value in ascending order to not duplicate equivalent requests, eg. getLOS(A, B) and getLOS(B, A)
        auto* raw1 = a1.lock().get();
        auto* raw2 = a2.lock().get();
        assert(raw1 != raw2);
        if (raw1 < raw2)
        {
            mActors = {a1, a2};
            mRawActors = {raw1, raw2};
        }
        else
        {
            mActors = {a2, a1};
            mRawActors = {raw2, raw1};
        }
    }

    bool operator==(const LOSRequest& lhs, const LOSRequest& rhs) noexcept
    {
        return lhs.mRawActors == rhs.mRawActors;
    }
}
