#include "physic.hpp"

#include <btBulletDynamicsCommon.h>
#include <btBulletCollisionCommon.h>
#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>

#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>

#include <OgreSceneManager.h>

#include <components/nifbullet/bulletnifloader.hpp>
#include <components/misc/stringops.hpp>

namespace
{

// Create a copy of the given collision shape (responsibility of user to delete the returned shape).
btCollisionShape *duplicateCollisionShape(btCollisionShape *shape)
{
    if(shape->isCompound())
    {
        btCompoundShape *comp = static_cast<btCompoundShape*>(shape);
        btCompoundShape *newShape = new btCompoundShape;

        int numShapes = comp->getNumChildShapes();
        for(int i = 0;i < numShapes;i++)
        {
            btCollisionShape *child = duplicateCollisionShape(comp->getChildShape(i));
            btTransform trans = comp->getChildTransform(i);
            newShape->addChildShape(trans, child);
        }

        return newShape;
    }

    if(btBvhTriangleMeshShape *trishape = dynamic_cast<btBvhTriangleMeshShape*>(shape))
    {
        btTriangleMesh* oldMesh = static_cast<btTriangleMesh*>(trishape->getMeshInterface());
        btTriangleMesh* newMesh = new btTriangleMesh(*oldMesh);
        NifBullet::TriangleMeshShape *newShape = new NifBullet::TriangleMeshShape(newMesh, true);

        return newShape;
    }

    throw std::logic_error(std::string("Unhandled Bullet shape duplication: ")+shape->getName());
}

void deleteShape(btCollisionShape* shape)
{
    if(shape!=NULL)
    {
        if(shape->isCompound())
        {
            btCompoundShape* ms = static_cast<btCompoundShape*>(shape);
            int a = ms->getNumChildShapes();
            for(int i=0; i <a;i++)
            {
                deleteShape(ms->getChildShape(i));
            }
        }
        delete shape;
    }
}

}

namespace OEngine {
namespace Physic
{

    PhysicActor::PhysicActor(const std::string &name, const std::string &mesh, PhysicEngine *engine, const Ogre::Vector3 &position, const Ogre::Quaternion &rotation, float scale)
      : mName(name), mEngine(engine), mMesh(mesh)
      , mBody(0), mOnGround(false), mInternalCollisionMode(true)
      , mExternalCollisionMode(true)
      , mForce(0.0f)
      , mScale(scale)
      , mWalkingOnWater(false)
      , mCanWaterWalk(false)
    {
        /*
        if (!NifBullet::getBoundingBox(mMesh, mHalfExtents, mMeshTranslation, mMeshOrientation))
        {
            mHalfExtents = Ogre::Vector3(0.f);
            mMeshTranslation = Ogre::Vector3(0.f);
            mMeshOrientation = Ogre::Quaternion::IDENTITY;
        }
        */
        /*
        // Use capsule shape only if base is square (nonuniform scaling apparently doesn't work on it)
        if (std::abs(mHalfExtents.x-mHalfExtents.y)<mHalfExtents.x*0.05 && mHalfExtents.z >= mHalfExtents.x)
        {
            // Could also be btCapsuleShapeZ, but the movement solver seems to have issues with it (jumping on slopes doesn't work)
            mShape.reset(new btCylinderShapeZ(BtOgre::Convert::toBullet(mHalfExtents)));
        }
        else
            mShape.reset(new btBoxShape(BtOgre::Convert::toBullet(mHalfExtents)));
        */

        mShape->setLocalScaling(btVector3(scale,scale,scale));

        btRigidBody::btRigidBodyConstructionInfo CI = btRigidBody::btRigidBodyConstructionInfo
                (0,0, mShape.get());
        mBody = new RigidBody(CI, name);
        mBody->mPlaceable = false;
        mBody->setCollisionFlags(btCollisionObject::CF_KINEMATIC_OBJECT);
        mBody->setActivationState(DISABLE_DEACTIVATION);

        setPosition(position);
        setRotation(rotation);

        updateCollisionMask();
    }

    PhysicActor::~PhysicActor()
    {
        if(mBody)
        {
            mEngine->mDynamicsWorld->removeRigidBody(mBody);
            delete mBody;
        }
    }

    void PhysicActor::enableCollisionMode(bool collision)
    {
        mInternalCollisionMode = collision;
    }

    void PhysicActor::enableCollisionBody(bool collision)
    {
        if (mExternalCollisionMode != collision)
        {
            mExternalCollisionMode = collision;
            updateCollisionMask();
        }
    }

    void PhysicActor::updateCollisionMask()
    {
        mEngine->mDynamicsWorld->removeRigidBody(mBody);
        int collisionMask = CollisionType_World | CollisionType_HeightMap;
        if (mExternalCollisionMode)
            collisionMask |= CollisionType_Actor | CollisionType_Projectile;
        if (mCanWaterWalk)
            collisionMask |= CollisionType_Water;
        mEngine->mDynamicsWorld->addRigidBody(mBody, CollisionType_Actor, collisionMask);
    }

    const Ogre::Vector3& PhysicActor::getPosition() const
    {
        return mPosition;
    }

    void PhysicActor::setPosition(const Ogre::Vector3 &position)
    {
        assert(mBody);

        mPosition = position;

        //btTransform tr = mBody->getWorldTransform();
        //Ogre::Quaternion meshrot = mMeshOrientation;
        //Ogre::Vector3 transrot = meshrot * (mMeshTranslation * mScale);
        //Ogre::Vector3 newPosition = transrot + position;

        //tr.setOrigin(BtOgre::Convert::toBullet(newPosition));
        //mBody->setWorldTransform(tr);
    }

    void PhysicActor::setRotation (const Ogre::Quaternion& rotation)
    {
        //btTransform tr = mBody->getWorldTransform();
        //tr.setRotation(BtOgre::Convert::toBullet(mMeshOrientation * rotation));
        //mBody->setWorldTransform(tr);
    }

    void PhysicActor::setScale(float scale)
    {
        mScale = scale;
        mShape->setLocalScaling(btVector3(scale,scale,scale));
        setPosition(mPosition);
    }

    Ogre::Vector3 PhysicActor::getHalfExtents() const
    {
        return mHalfExtents * mScale;
    }

    void PhysicActor::setInertialForce(const Ogre::Vector3 &force)
    {
        mForce = force;
    }

    void PhysicActor::setOnGround(bool grounded)
    {
        mOnGround = grounded;
    }

    bool PhysicActor::isWalkingOnWater() const
    {
        return mWalkingOnWater;
    }

    void PhysicActor::setWalkingOnWater(bool walkingOnWater)
    {
        mWalkingOnWater = walkingOnWater;
    }

    void PhysicActor::setCanWaterWalk(bool waterWalk)
    {
        if (waterWalk != mCanWaterWalk)
        {
            mCanWaterWalk = waterWalk;
            updateCollisionMask();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////


    RigidBody::RigidBody(btRigidBody::btRigidBodyConstructionInfo& CI,std::string name)
        : btRigidBody(CI)
        , mName(name)
        , mPlaceable(false)
    {
    }

    RigidBody::~RigidBody()
    {
        delete getMotionState();
    }



    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////



    PhysicEngine::PhysicEngine(BulletShapeLoader* shapeLoader)
    {
        // Set up the collision configuration and dispatcher
        collisionConfiguration = new btDefaultCollisionConfiguration();
        dispatcher = new btCollisionDispatcher(collisionConfiguration);

        // The actual physics solver
        solver = new btSequentialImpulseConstraintSolver;

        broadphase = new btDbvtBroadphase();

        // The world.
        mDynamicsWorld = new btDiscreteDynamicsWorld(dispatcher,broadphase,solver,collisionConfiguration);

        // Don't update AABBs of all objects every frame. Most objects in MW are static, so we don't need this.
        // Should a "static" object ever be moved, we have to update its AABB manually using DynamicsWorld::updateSingleAabb.
        mDynamicsWorld->setForceUpdateAllAabbs(false);

        mDynamicsWorld->setGravity(btVector3(0,0,-10));

        mShapeLoader = shapeLoader;
    }

    PhysicEngine::~PhysicEngine()
    {
        for (std::map<RigidBody*, AnimatedShapeInstance>::iterator it = mAnimatedShapes.begin(); it != mAnimatedShapes.end(); ++it)
            deleteShape(it->second.mCompound);

        HeightFieldContainer::iterator hf_it = mHeightFieldMap.begin();
        for (; hf_it != mHeightFieldMap.end(); ++hf_it)
        {
            mDynamicsWorld->removeRigidBody(hf_it->second.mBody);
            delete hf_it->second.mShape;
            delete hf_it->second.mBody;
        }

        RigidBodyContainer::iterator rb_it = mCollisionObjectMap.begin();
        for (; rb_it != mCollisionObjectMap.end(); ++rb_it)
        {
            if (rb_it->second != NULL)
            {
                mDynamicsWorld->removeRigidBody(rb_it->second);

                delete rb_it->second;
                rb_it->second = NULL;
            }
        }

        PhysicActorContainer::iterator pa_it = mActorMap.begin();
        for (; pa_it != mActorMap.end(); ++pa_it)
        {
            if (pa_it->second != NULL)
            {
                delete pa_it->second;
                pa_it->second = NULL;
            }
        }

        delete mDynamicsWorld;
        delete solver;
        delete collisionConfiguration;
        delete dispatcher;
        delete broadphase;
        delete mShapeLoader;
    }

    void PhysicEngine::addHeightField(float* heights,
        int x, int y, float yoffset,
        float triSize, float sqrtVerts)
    {
        const std::string name = "HeightField_"
            + boost::lexical_cast<std::string>(x) + "_"
            + boost::lexical_cast<std::string>(y);

        // find the minimum and maximum heights (needed for bullet)
        float minh = heights[0];
        float maxh = heights[0];
        for (int i=0; i<sqrtVerts*sqrtVerts; ++i)
        {
            float h = heights[i];

            if (h>maxh) maxh = h;
            if (h<minh) minh = h;
        }

        btHeightfieldTerrainShape* hfShape = new btHeightfieldTerrainShape(
            static_cast<int>(sqrtVerts), static_cast<int>(sqrtVerts), heights, 1,
            minh, maxh, 2,
            PHY_FLOAT,true);

        hfShape->setUseDiamondSubdivision(true);

        btVector3 scl(triSize, triSize, 1);
        hfShape->setLocalScaling(scl);

        btRigidBody::btRigidBodyConstructionInfo CI = btRigidBody::btRigidBodyConstructionInfo(0,0,hfShape);
        RigidBody* body = new RigidBody(CI,name);
        body->getWorldTransform().setOrigin(btVector3( (x+0.5f)*triSize*(sqrtVerts-1), (y+0.5f)*triSize*(sqrtVerts-1), (maxh+minh)/2.f));

        HeightField hf;
        hf.mBody = body;
        hf.mShape = hfShape;

        mHeightFieldMap [name] = hf;

        mDynamicsWorld->addRigidBody(body,CollisionType_HeightMap,
                                    CollisionType_Actor|CollisionType_Projectile);
    }

    void PhysicEngine::removeHeightField(int x, int y)
    {
        const std::string name = "HeightField_"
            + boost::lexical_cast<std::string>(x) + "_"
            + boost::lexical_cast<std::string>(y);

        HeightFieldContainer::iterator it = mHeightFieldMap.find(name);
        if (it == mHeightFieldMap.end())
            return;

        const HeightField& hf = it->second;

        mDynamicsWorld->removeRigidBody(hf.mBody);
        delete hf.mShape;
        delete hf.mBody;

        mHeightFieldMap.erase(it);
    }

    void PhysicEngine::adjustRigidBody(RigidBody* body, const Ogre::Vector3 &position, const Ogre::Quaternion &rotation,
        const Ogre::Vector3 &scaledBoxTranslation, const Ogre::Quaternion &boxRotation)
    {
        btTransform tr;
        Ogre::Quaternion boxrot = rotation * boxRotation;
        Ogre::Vector3 transrot = boxrot * scaledBoxTranslation;
        Ogre::Vector3 newPosition = transrot + position;

        tr.setOrigin(btVector3(newPosition.x, newPosition.y, newPosition.z));
        tr.setRotation(btQuaternion(boxrot.x,boxrot.y,boxrot.z,boxrot.w));
        body->setWorldTransform(tr);
    }
    void PhysicEngine::boxAdjustExternal(const std::string &mesh, RigidBody* body,
        float scale, const Ogre::Vector3 &position, const Ogre::Quaternion &rotation)
    {
        std::string sid = (boost::format("%07.3f") % scale).str();
        std::string outputstring = mesh + sid;

        //get the shape from the .nif
        mShapeLoader->load(outputstring,"General");
        BulletShapeManager::getSingletonPtr()->load(outputstring,"General");
        BulletShapePtr shape = BulletShapeManager::getSingleton().getByName(outputstring,"General");

        //adjustRigidBody(body, position, rotation, shape->mBoxTranslation * scale, shape->mBoxRotation);
    }

    RigidBody* PhysicEngine::createAndAdjustRigidBody(const std::string &mesh, const std::string &name,
        float scale, const Ogre::Vector3 &position, const Ogre::Quaternion &rotation,
        Ogre::Vector3* scaledBoxTranslation, Ogre::Quaternion* boxRotation, bool placeable)
    {
        std::string sid = (boost::format("%07.3f") % scale).str();
        std::string outputstring = mesh + sid;

        //get the shape from the .nif
        mShapeLoader->load(outputstring,"General");
        BulletShapeManager::getSingletonPtr()->load(outputstring,"General");
        BulletShapePtr shape = BulletShapeManager::getSingleton().getByName(outputstring,"General");

        // TODO: add option somewhere to enable collision for placeable meshes

        if (placeable && shape->mCollisionShape)
            return NULL;

        if (!shape->mCollisionShape)
            return NULL;

        btCollisionShape* collisionShape = shape->mCollisionShape;

        // If this is an animated compound shape, we must duplicate it so we can animate
        // multiple instances independently.
        if (!shape->mAnimatedShapes.empty())
            collisionShape = duplicateCollisionShape(collisionShape);

        collisionShape->setLocalScaling( btVector3(scale,scale,scale));

        //create the real body
        btRigidBody::btRigidBodyConstructionInfo CI = btRigidBody::btRigidBodyConstructionInfo
                (0,0, collisionShape);
        RigidBody* body = new RigidBody(CI,name);
        body->mPlaceable = placeable;

        if (!shape->mAnimatedShapes.empty())
        {
            AnimatedShapeInstance instance;
            instance.mAnimatedShapes = shape->mAnimatedShapes;
            instance.mCompound = collisionShape;
            mAnimatedShapes[body] = instance;
        }

        //if(scaledBoxTranslation != 0)
        //    *scaledBoxTranslation = shape->mBoxTranslation * scale;
        //if(boxRotation != 0)
        //    *boxRotation = shape->mBoxRotation;

        //adjustRigidBody(body, position, rotation, shape->mBoxTranslation * scale, shape->mBoxRotation);

        assert (mCollisionObjectMap.find(name) == mCollisionObjectMap.end());
        mCollisionObjectMap[name] = body;
        mDynamicsWorld->addRigidBody(body,CollisionType_World,CollisionType_Actor|CollisionType_HeightMap);

        return body;
    }

    void PhysicEngine::removeRigidBody(const std::string &name)
    {
        RigidBodyContainer::iterator it = mCollisionObjectMap.find(name);
        if (it != mCollisionObjectMap.end() )
        {
            RigidBody* body = it->second;
            if(body != NULL)
            {
                mDynamicsWorld->removeRigidBody(body);
            }
        }
    }

    void PhysicEngine::deleteRigidBody(const std::string &name)
    {
        RigidBodyContainer::iterator it = mCollisionObjectMap.find(name);
        if (it != mCollisionObjectMap.end() )
        {
            RigidBody* body = it->second;

            if(body != NULL)
            {
                if (mAnimatedShapes.find(body) != mAnimatedShapes.end())
                    deleteShape(mAnimatedShapes[body].mCompound);
                mAnimatedShapes.erase(body);

                delete body;
            }
            mCollisionObjectMap.erase(it);
        }
    }

    RigidBody* PhysicEngine::getRigidBody(const std::string &name)
    {
        RigidBodyContainer::iterator it = mCollisionObjectMap.find(name);
        if (it != mCollisionObjectMap.end() )
        {
            RigidBody* body = mCollisionObjectMap[name];
            return body;
        }
        else
        {
            return NULL;
        }
    }

    class ContactTestResultCallback : public btCollisionWorld::ContactResultCallback
    {
    public:
        std::vector<std::string> mResult;

        // added in bullet 2.81
        // this is just a quick hack, as there does not seem to be a BULLET_VERSION macro?
#if defined(BT_COLLISION_OBJECT_WRAPPER_H)
        virtual	btScalar addSingleResult(btManifoldPoint& cp,
                                            const btCollisionObjectWrapper* colObj0Wrap,int partId0,int index0,
                                            const btCollisionObjectWrapper* colObj1Wrap,int partId1,int index1)
        {
            const RigidBody* body = dynamic_cast<const RigidBody*>(colObj0Wrap->m_collisionObject);
            if (body)
                mResult.push_back(body->mName);

            return 0.f;
        }
#else
        virtual btScalar addSingleResult(btManifoldPoint& cp, const btCollisionObject* col0, int partId0, int index0,
                                         const btCollisionObject* col1, int partId1, int index1)
        {
            const RigidBody* body = dynamic_cast<const RigidBody*>(col0);
            if (body)
                mResult.push_back(body->mName);

            return 0.f;
        }
#endif
    };

    class DeepestNotMeContactTestResultCallback : public btCollisionWorld::ContactResultCallback
    {
        const std::string &mFilter;
        // Store the real origin, since the shape's origin is its center
        btVector3 mOrigin;

    public:
        const RigidBody *mObject;
        btVector3 mContactPoint;
        btScalar mLeastDistSqr;

        DeepestNotMeContactTestResultCallback(const std::string &filter, const btVector3 &origin)
          : mFilter(filter), mOrigin(origin), mObject(0), mContactPoint(0,0,0),
            mLeastDistSqr(std::numeric_limits<float>::max())
        { }

#if defined(BT_COLLISION_OBJECT_WRAPPER_H)
        virtual btScalar addSingleResult(btManifoldPoint& cp,
                                         const btCollisionObjectWrapper* col0Wrap,int partId0,int index0,
                                         const btCollisionObjectWrapper* col1Wrap,int partId1,int index1)
        {
            const RigidBody* body = dynamic_cast<const RigidBody*>(col1Wrap->m_collisionObject);
            if(body && body->mName != mFilter)
            {
                btScalar distsqr = mOrigin.distance2(cp.getPositionWorldOnA());
                if(!mObject || distsqr < mLeastDistSqr)
                {
                    mObject = body;
                    mLeastDistSqr = distsqr;
                    mContactPoint = cp.getPositionWorldOnA();
                }
            }

            return 0.f;
        }
#else
        virtual btScalar addSingleResult(btManifoldPoint& cp,
                                         const btCollisionObject* col0, int partId0, int index0,
                                         const btCollisionObject* col1, int partId1, int index1)
        {
            const RigidBody* body = dynamic_cast<const RigidBody*>(col1);
            if(body && body->mName != mFilter)
            {
                btScalar distsqr = mOrigin.distance2(cp.getPositionWorldOnA());
                if(!mObject || distsqr < mLeastDistSqr)
                {
                    mObject = body;
                    mLeastDistSqr = distsqr;
                    mContactPoint = cp.getPositionWorldOnA();
                }
            }

            return 0.f;
        }
#endif
    };


    std::vector<std::string> PhysicEngine::getCollisions(const std::string& name, int collisionGroup, int collisionMask)
    {
        RigidBody* body = getRigidBody(name);
        ContactTestResultCallback callback;
        callback.m_collisionFilterGroup = collisionGroup;
        callback.m_collisionFilterMask = collisionMask;
        mDynamicsWorld->contactTest(body, callback);
        return callback.mResult;
    }


    std::pair<const RigidBody*,btVector3> PhysicEngine::getFilteredContact(const std::string &filter,
                                                                           const btVector3 &origin,
                                                                           btCollisionObject *object)
    {
        DeepestNotMeContactTestResultCallback callback(filter, origin);
        callback.m_collisionFilterGroup = CollisionType_Actor;
        callback.m_collisionFilterMask = CollisionType_World | CollisionType_HeightMap | CollisionType_Actor;
        mDynamicsWorld->contactTest(object, callback);
        return std::make_pair(callback.mObject, callback.mContactPoint);
    }

    void PhysicEngine::stepSimulation(double deltaT)
    {
        // This seems to be needed for character controller objects
        mDynamicsWorld->stepSimulation(static_cast<btScalar>(deltaT), 10, 1 / 60.0f);
    }

    void PhysicEngine::addCharacter(const std::string &name, const std::string &mesh,
        const Ogre::Vector3 &position, float scale, const Ogre::Quaternion &rotation)
    {
        // Remove character with given name, so we don't make memory
        // leak when character would be added twice
        removeCharacter(name);

        PhysicActor* newActor = new PhysicActor(name, mesh, this, position, rotation, scale);

        mActorMap[name] = newActor;
    }

    void PhysicEngine::removeCharacter(const std::string &name)
    {
        PhysicActorContainer::iterator it = mActorMap.find(name);
        if (it != mActorMap.end() )
        {
            PhysicActor* act = it->second;
            if(act != NULL)
            {
                delete act;
            }
            mActorMap.erase(it);
        }
    }

    PhysicActor* PhysicEngine::getCharacter(const std::string &name)
    {
        PhysicActorContainer::iterator it = mActorMap.find(name);
        if (it != mActorMap.end() )
        {
            PhysicActor* act = mActorMap[name];
            return act;
        }
        else
        {
            return 0;
        }
    }

    std::pair<std::string,float> PhysicEngine::rayTest(const btVector3 &from, const btVector3 &to, bool ignoreHeightMap, Ogre::Vector3* normal)
    {
        std::string name = "";
        float d = -1;

        btCollisionWorld::ClosestRayResultCallback resultCallback1(from, to);
        resultCallback1.m_collisionFilterGroup = 0xff;
        resultCallback1.m_collisionFilterMask = CollisionType_World;

        if(!ignoreHeightMap)
            resultCallback1.m_collisionFilterMask = resultCallback1.m_collisionFilterMask | CollisionType_HeightMap;
        mDynamicsWorld->rayTest(from, to, resultCallback1);
        if (resultCallback1.hasHit())
        {
            name = static_cast<const RigidBody&>(*resultCallback1.m_collisionObject).mName;
            d = resultCallback1.m_closestHitFraction;
            if (normal)
                *normal = Ogre::Vector3(resultCallback1.m_hitNormalWorld.x(),
                                        resultCallback1.m_hitNormalWorld.y(),
                                        resultCallback1.m_hitNormalWorld.z());
        }

        return std::pair<std::string,float>(name,d);
    }

    // callback that ignores player in results
    struct	OurClosestConvexResultCallback : public btCollisionWorld::ClosestConvexResultCallback
    {
    public:
        OurClosestConvexResultCallback(const btVector3&	convexFromWorld,const btVector3&	convexToWorld)
            : btCollisionWorld::ClosestConvexResultCallback(convexFromWorld, convexToWorld) {}

        virtual	btScalar	addSingleResult(btCollisionWorld::LocalConvexResult& convexResult,bool normalInWorldSpace)
        {
            if (const RigidBody* body = dynamic_cast<const RigidBody*>(convexResult.m_hitCollisionObject))
                if (body->mName == "player")
                    return 0;
            return btCollisionWorld::ClosestConvexResultCallback::addSingleResult(convexResult, normalInWorldSpace);
        }
    };

    std::pair<bool, float> PhysicEngine::sphereCast (float radius, btVector3& from, btVector3& to)
    {
        OurClosestConvexResultCallback callback(from, to);
        callback.m_collisionFilterGroup = 0xff;
        callback.m_collisionFilterMask = OEngine::Physic::CollisionType_World|OEngine::Physic::CollisionType_HeightMap;

        btSphereShape shape(radius);
        const btQuaternion btrot(0.0f, 0.0f, 0.0f);

        btTransform from_ (btrot, from);
        btTransform to_ (btrot, to);

        mDynamicsWorld->convexSweepTest(&shape, from_, to_, callback);

        if (callback.hasHit())
            return std::make_pair(true, callback.m_closestHitFraction);
        else
            return std::make_pair(false, 1.0f);
    }

}
}
