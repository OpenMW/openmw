#include "physic.hpp"
#include <btBulletDynamicsCommon.h>
#include <btBulletCollisionCommon.h>
#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>
#include <components/nifbullet/bulletnifloader.hpp>
#include "OgreRoot.h"
#include "btKinematicCharacterController.h"
#include "BtOgrePG.h"
#include "BtOgreGP.h"
#include "BtOgreExtras.h"

#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>

namespace OEngine {
namespace Physic
{

    PhysicActor::PhysicActor(const std::string &name, const std::string &mesh, PhysicEngine *engine, const Ogre::Vector3 &position, const Ogre::Quaternion &rotation, float scale)
      : mName(name), mEngine(engine), mMesh(mesh), mBoxScaledTranslation(0,0,0), mBoxRotationInverse(0,0,0,0)
      , mBody(0), mRaycastingBody(0), mOnGround(false), mCollisionMode(true), mBoxRotation(0,0,0,0)
      , mForce(0.0f)
    {
        mBody = mEngine->createAndAdjustRigidBody(mMesh, mName, scale, position, rotation, &mBoxScaledTranslation, &mBoxRotation);
        mRaycastingBody = mEngine->createAndAdjustRigidBody(mMesh, mName, scale, position, rotation, &mBoxScaledTranslation, &mBoxRotation, true);
        Ogre::Quaternion inverse = mBoxRotation.Inverse();
        mBoxRotationInverse = btQuaternion(inverse.x, inverse.y, inverse.z,inverse.w);
        mEngine->addRigidBody(mBody, false, mRaycastingBody,true);  //Add rigid body to dynamics world, but do not add to object map
    }

    PhysicActor::~PhysicActor()
    {
        if(mBody)
        {
            mEngine->dynamicsWorld->removeRigidBody(mBody);
            delete mBody;
        }
        if(mRaycastingBody)
        {
            mEngine->dynamicsWorld->removeRigidBody(mRaycastingBody);
            delete mRaycastingBody;
        }
    }

    void PhysicActor::enableCollisions(bool collision)
    {
        assert(mBody);
        if(collision && !mCollisionMode) enableCollisionBody();
        if(!collision && mCollisionMode) disableCollisionBody();
        mCollisionMode = collision;
    }


    void PhysicActor::setPosition(const Ogre::Vector3 &pos)
    {
        assert(mBody);
        if(pos != getPosition())
        {
            mEngine->adjustRigidBody(mBody, pos, getRotation(), mBoxScaledTranslation, mBoxRotation);
            mEngine->adjustRigidBody(mRaycastingBody, pos, getRotation(), mBoxScaledTranslation, mBoxRotation);
        }
    }

    void PhysicActor::setRotation(const Ogre::Quaternion &quat)
    {
        assert(mBody);
        if(!quat.equals(getRotation(), Ogre::Radian(0))){
            mEngine->adjustRigidBody(mBody, getPosition(), quat, mBoxScaledTranslation, mBoxRotation);
            mEngine->adjustRigidBody(mRaycastingBody, getPosition(), quat, mBoxScaledTranslation, mBoxRotation);

        }
    }


    Ogre::Vector3 PhysicActor::getPosition()
    {
        assert(mBody);
        btVector3 vec = mBody->getWorldTransform().getOrigin();
        Ogre::Quaternion rotation = Ogre::Quaternion(mBody->getWorldTransform().getRotation().getW(), mBody->getWorldTransform().getRotation().getX(),
            mBody->getWorldTransform().getRotation().getY(), mBody->getWorldTransform().getRotation().getZ());
        Ogre::Vector3 transrot = rotation * mBoxScaledTranslation;
        Ogre::Vector3 visualPosition = Ogre::Vector3(vec.getX(), vec.getY(), vec.getZ()) - transrot;
        return visualPosition;
    }

    Ogre::Quaternion PhysicActor::getRotation()
    {
        assert(mBody);
        btQuaternion quat = mBody->getWorldTransform().getRotation() * mBoxRotationInverse;
        return Ogre::Quaternion(quat.getW(), quat.getX(), quat.getY(), quat.getZ());
    }

    void PhysicActor::setScale(float scale){
        //We only need to change the scaled box translation, box rotations remain the same.
        assert(mBody);
        mBoxScaledTranslation = mBoxScaledTranslation / mBody->getCollisionShape()->getLocalScaling().getX();
        mBoxScaledTranslation *= scale;
        Ogre::Vector3 pos = getPosition();
        Ogre::Quaternion rot = getRotation();
        if(mBody){
            mEngine->dynamicsWorld->removeRigidBody(mBody);
            mEngine->dynamicsWorld->removeRigidBody(mRaycastingBody);
            delete mBody;
            delete mRaycastingBody;
        }
        //Create the newly scaled rigid body
        mBody = mEngine->createAndAdjustRigidBody(mMesh, mName, scale, pos, rot);
        mRaycastingBody = mEngine->createAndAdjustRigidBody(mMesh, mName, scale, pos, rot, 0, 0, true);
        mEngine->addRigidBody(mBody, false, mRaycastingBody,true);  //Add rigid body to dynamics world, but do not add to object map
    }

    Ogre::Vector3 PhysicActor::getHalfExtents() const
    {
        if(mBody)
        {
            btBoxShape *box = static_cast<btBoxShape*>(mBody->getCollisionShape());
            if(box != NULL)
            {
                btVector3 size = box->getHalfExtentsWithMargin();
                return Ogre::Vector3(size.getX(), size.getY(), size.getZ());
            }
        }
        return Ogre::Vector3(0.0f);
    }

    void PhysicActor::setInertialForce(const Ogre::Vector3 &force)
    {
        mForce = force;
    }

    void PhysicActor::setOnGround(bool grounded)
    {
        mOnGround = grounded;
    }

    void PhysicActor::disableCollisionBody()
    {
        mEngine->dynamicsWorld->removeRigidBody(mBody);
    }

    void PhysicActor::enableCollisionBody()
    {
        mEngine->dynamicsWorld->addRigidBody(mBody,CollisionType_Actor,CollisionType_World|CollisionType_HeightMap);
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



    PhysicEngine::PhysicEngine(BulletShapeLoader* shapeLoader) :
        mDebugActive(0)
      , mSceneMgr(NULL)
    {
        // Set up the collision configuration and dispatcher
        collisionConfiguration = new btDefaultCollisionConfiguration();
        dispatcher = new btCollisionDispatcher(collisionConfiguration);

        // The actual physics solver
        solver = new btSequentialImpulseConstraintSolver;

        //btOverlappingPairCache* pairCache = new btSortedOverlappingPairCache();
        pairCache = new btSortedOverlappingPairCache();

        //pairCache->setInternalGhostPairCallback( new btGhostPairCallback() );

        broadphase = new btDbvtBroadphase();

        // The world.
        dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher,broadphase,solver,collisionConfiguration);
        dynamicsWorld->setGravity(btVector3(0,0,-10));

        if(BulletShapeManager::getSingletonPtr() == NULL)
        {
            new BulletShapeManager();
        }
        //TODO:singleton?
        mShapeLoader = shapeLoader;

        isDebugCreated = false;
        mDebugDrawer = NULL;
    }

    void PhysicEngine::createDebugRendering()
    {
        if(!isDebugCreated)
        {
            Ogre::SceneNode* node = mSceneMgr->getRootSceneNode()->createChildSceneNode();
            mDebugDrawer = new BtOgre::DebugDrawer(node, dynamicsWorld);
            dynamicsWorld->setDebugDrawer(mDebugDrawer);
            isDebugCreated = true;
            dynamicsWorld->debugDrawWorld();
        }
    }

    void PhysicEngine::setDebugRenderingMode(int mode)
    {
        if(!isDebugCreated)
        {
            createDebugRendering();
        }
        mDebugDrawer->setDebugMode(mode);
        mDebugActive = mode;
    }

    bool  PhysicEngine::toggleDebugRendering()
    {
        setDebugRenderingMode(!mDebugActive);
        return mDebugActive;
    }

    void PhysicEngine::setSceneManager(Ogre::SceneManager* sceneMgr)
    {
        mSceneMgr = sceneMgr;
    }

    PhysicEngine::~PhysicEngine()
    {
        HeightFieldContainer::iterator hf_it = mHeightFieldMap.begin();
        for (; hf_it != mHeightFieldMap.end(); ++hf_it)
        {
            dynamicsWorld->removeRigidBody(hf_it->second.mBody);
            delete hf_it->second.mShape;
            delete hf_it->second.mBody;
        }

        RigidBodyContainer::iterator rb_it = mCollisionObjectMap.begin();
        for (; rb_it != mCollisionObjectMap.end(); ++rb_it)
        {
            if (rb_it->second != NULL)
            {
                dynamicsWorld->removeRigidBody(rb_it->second);

                delete rb_it->second;
                rb_it->second = NULL;
            }
        }
        rb_it = mRaycastingObjectMap.begin();
        for (; rb_it != mRaycastingObjectMap.end(); ++rb_it)
        {
            if (rb_it->second != NULL)
            {
                dynamicsWorld->removeRigidBody(rb_it->second);

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

        delete mDebugDrawer;

        delete dynamicsWorld;
        delete solver;
        delete collisionConfiguration;
        delete dispatcher;
        delete broadphase;
        delete pairCache;
        delete mShapeLoader;

        delete BulletShapeManager::getSingletonPtr();
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
            sqrtVerts, sqrtVerts, heights, 1,
            minh, maxh, 2,
            PHY_FLOAT,true);

        hfShape->setUseDiamondSubdivision(true);

        btVector3 scl(triSize, triSize, 1);
        hfShape->setLocalScaling(scl);

        btRigidBody::btRigidBodyConstructionInfo CI = btRigidBody::btRigidBodyConstructionInfo(0,0,hfShape);
        RigidBody* body = new RigidBody(CI,name);
        body->getWorldTransform().setOrigin(btVector3( (x+0.5)*triSize*(sqrtVerts-1), (y+0.5)*triSize*(sqrtVerts-1), (maxh+minh)/2.f));

        HeightField hf;
        hf.mBody = body;
        hf.mShape = hfShape;

        mHeightFieldMap [name] = hf;

        dynamicsWorld->addRigidBody(body,CollisionType_HeightMap|CollisionType_Raycasting,
                                    CollisionType_World|CollisionType_Actor|CollisionType_Raycasting);
    }

    void PhysicEngine::removeHeightField(int x, int y)
    {
        const std::string name = "HeightField_"
            + boost::lexical_cast<std::string>(x) + "_"
            + boost::lexical_cast<std::string>(y);

        HeightField hf = mHeightFieldMap [name];

        dynamicsWorld->removeRigidBody(hf.mBody);
        delete hf.mShape;
        delete hf.mBody;

        mHeightFieldMap.erase(name);
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
        //std::cout << "The string" << outputstring << "\n";

        //get the shape from the .nif
        mShapeLoader->load(outputstring,"General");
        BulletShapeManager::getSingletonPtr()->load(outputstring,"General");
        BulletShapePtr shape = BulletShapeManager::getSingleton().getByName(outputstring,"General");

        adjustRigidBody(body, position, rotation, shape->mBoxTranslation * scale, shape->mBoxRotation);
    }

    RigidBody* PhysicEngine::createAndAdjustRigidBody(const std::string &mesh, const std::string &name,
        float scale, const Ogre::Vector3 &position, const Ogre::Quaternion &rotation,
        Ogre::Vector3* scaledBoxTranslation, Ogre::Quaternion* boxRotation, bool raycasting, bool placeable)
    {
        std::string sid = (boost::format("%07.3f") % scale).str();
        std::string outputstring = mesh + sid;

        //get the shape from the .nif
        mShapeLoader->load(outputstring,"General");
        BulletShapeManager::getSingletonPtr()->load(outputstring,"General");
        BulletShapePtr shape = BulletShapeManager::getSingleton().getByName(outputstring,"General");

        if (placeable && !raycasting && shape->mCollisionShape && !shape->mHasCollisionNode)
            return NULL;

        if (!shape->mCollisionShape && !raycasting)
            return NULL;
        if (!shape->mRaycastingShape && raycasting)
            return NULL;

        if (!raycasting)
            shape->mCollisionShape->setLocalScaling( btVector3(scale,scale,scale));
        else
            shape->mRaycastingShape->setLocalScaling( btVector3(scale,scale,scale));

        //create the real body
        btRigidBody::btRigidBodyConstructionInfo CI = btRigidBody::btRigidBodyConstructionInfo
                (0,0, raycasting ? shape->mRaycastingShape : shape->mCollisionShape);
        RigidBody* body = new RigidBody(CI,name);
        body->mPlaceable = placeable;

        if(scaledBoxTranslation != 0)
            *scaledBoxTranslation = shape->mBoxTranslation * scale;
        if(boxRotation != 0)
            *boxRotation = shape->mBoxRotation;

        adjustRigidBody(body, position, rotation, shape->mBoxTranslation * scale, shape->mBoxRotation);

        return body;

    }

    void PhysicEngine::addRigidBody(RigidBody* body, bool addToMap, RigidBody* raycastingBody,bool actor)
    {
        if(!body && !raycastingBody)
            return; // nothing to do

        const std::string& name = (body ? body->mName : raycastingBody->mName);

        if (body){
            if(actor) dynamicsWorld->addRigidBody(body,CollisionType_Actor,CollisionType_World|CollisionType_HeightMap);
            else dynamicsWorld->addRigidBody(body,CollisionType_World,CollisionType_World|CollisionType_Actor|CollisionType_HeightMap);
        }

        if (raycastingBody)
            dynamicsWorld->addRigidBody(raycastingBody,CollisionType_Raycasting,CollisionType_Raycasting|CollisionType_World);

        if(addToMap){
            removeRigidBody(name);
            deleteRigidBody(name);

            if (body)
                mCollisionObjectMap[name] = body;
            if (raycastingBody)
                mRaycastingObjectMap[name] = raycastingBody;
        }
    }

    void PhysicEngine::removeRigidBody(const std::string &name)
    {
        RigidBodyContainer::iterator it = mCollisionObjectMap.find(name);
        if (it != mCollisionObjectMap.end() )
        {
            RigidBody* body = it->second;
            if(body != NULL)
            {
                dynamicsWorld->removeRigidBody(body);
            }
        }
        it = mRaycastingObjectMap.find(name);
        if (it != mRaycastingObjectMap.end() )
        {
            RigidBody* body = it->second;
            if(body != NULL)
            {
                dynamicsWorld->removeRigidBody(body);
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
                delete body;
            }
            mCollisionObjectMap.erase(it);
        }
        it = mRaycastingObjectMap.find(name);
        if (it != mRaycastingObjectMap.end() )
        {
            RigidBody* body = it->second;

            if(body != NULL)
            {
                delete body;
            }
            mRaycastingObjectMap.erase(it);
        }
    }

    RigidBody* PhysicEngine::getRigidBody(const std::string &name, bool raycasting)
    {
        RigidBodyContainer* map = raycasting ? &mRaycastingObjectMap : &mCollisionObjectMap;
        RigidBodyContainer::iterator it = map->find(name);
        if (it != map->end() )
        {
            RigidBody* body = (*map)[name];
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
            if (body && !(colObj0Wrap->m_collisionObject->getBroadphaseHandle()->m_collisionFilterGroup
                          & CollisionType_Raycasting))
                mResult.push_back(body->mName);

            return 0.f;
        }
#else
        virtual btScalar addSingleResult(btManifoldPoint& cp, const btCollisionObject* col0, int partId0, int index0,
                                         const btCollisionObject* col1, int partId1, int index1)
        {
            const RigidBody* body = dynamic_cast<const RigidBody*>(col0);
            if (body && !(col0->getBroadphaseHandle()->m_collisionFilterGroup
                          & CollisionType_Raycasting))
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


    std::vector<std::string> PhysicEngine::getCollisions(const std::string& name)
    {
        RigidBody* body = getRigidBody(name);
        ContactTestResultCallback callback;
        dynamicsWorld->contactTest(body, callback);
        return callback.mResult;
    }


    std::pair<const RigidBody*,btVector3> PhysicEngine::getFilteredContact(const std::string &filter,
                                                                           const btVector3 &origin,
                                                                           btCollisionObject *object)
    {
        DeepestNotMeContactTestResultCallback callback(filter, origin);
        dynamicsWorld->contactTest(object, callback);
        return std::make_pair(callback.mObject, callback.mContactPoint);
    }


    void PhysicEngine::stepSimulation(double deltaT)
    {
        // This seems to be needed for character controller objects
        dynamicsWorld->stepSimulation(deltaT,10, 1/60.0);
        if(isDebugCreated)
        {
            mDebugDrawer->step();
        }
    }

    void PhysicEngine::addCharacter(const std::string &name, const std::string &mesh,
        const Ogre::Vector3 &position, float scale, const Ogre::Quaternion &rotation)
    {
        // Remove character with given name, so we don't make memory
        // leak when character would be added twice
        removeCharacter(name);

        PhysicActor* newActor = new PhysicActor(name, mesh, this, position, rotation, scale);


        //dynamicsWorld->addAction( newActor->mCharacter );
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

    void PhysicEngine::emptyEventLists(void)
    {
    }

    std::pair<std::string,float> PhysicEngine::rayTest(btVector3& from,btVector3& to,bool raycastingObjectOnly,bool ignoreHeightMap)
    {
        std::string name = "";
        float d = -1;

        btCollisionWorld::ClosestRayResultCallback resultCallback1(from, to);
        if(raycastingObjectOnly)
            resultCallback1.m_collisionFilterMask = CollisionType_Raycasting;
        else
            resultCallback1.m_collisionFilterMask = CollisionType_World;

        if(!ignoreHeightMap)
            resultCallback1.m_collisionFilterMask = resultCallback1.m_collisionFilterMask | CollisionType_HeightMap;
        dynamicsWorld->rayTest(from, to, resultCallback1);
        if (resultCallback1.hasHit())
        {
            name = static_cast<const RigidBody&>(*resultCallback1.m_collisionObject).mName;
            d = resultCallback1.m_closestHitFraction;;
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
        callback.m_collisionFilterMask = OEngine::Physic::CollisionType_World|OEngine::Physic::CollisionType_HeightMap;

        btSphereShape shape(radius);
        const btQuaternion btrot(0.0f, 0.0f, 0.0f);

        btTransform from_ (btrot, from);
        btTransform to_ (btrot, to);

        dynamicsWorld->convexSweepTest(&shape, from_, to_, callback);

        if (callback.hasHit())
            return std::make_pair(true, callback.m_closestHitFraction);
        else
            return std::make_pair(false, 1);
    }

    std::vector< std::pair<float, std::string> > PhysicEngine::rayTest2(btVector3& from, btVector3& to)
    {
        MyRayResultCallback resultCallback1;
        resultCallback1.m_collisionFilterMask = CollisionType_Raycasting;
        dynamicsWorld->rayTest(from, to, resultCallback1);
        std::vector< std::pair<float, const btCollisionObject*> > results = resultCallback1.results;

        std::vector< std::pair<float, std::string> > results2;

        for (std::vector< std::pair<float, const btCollisionObject*> >::iterator it=results.begin();
            it != results.end(); ++it)
        {
            results2.push_back( std::make_pair( (*it).first, static_cast<const RigidBody&>(*(*it).second).mName ) );
        }

        std::sort(results2.begin(), results2.end(), MyRayResultCallback::cmp);

        return results2;
    }

    void PhysicEngine::getObjectAABB(const std::string &mesh, float scale, btVector3 &min, btVector3 &max)
    {
        std::string sid = (boost::format("%07.3f") % scale).str();
        std::string outputstring = mesh + sid;

        mShapeLoader->load(outputstring, "General");
        BulletShapeManager::getSingletonPtr()->load(outputstring, "General");
        BulletShapePtr shape =
            BulletShapeManager::getSingleton().getByName(outputstring, "General");

        btTransform trans;
        trans.setIdentity();

        if (shape->mRaycastingShape)
            shape->mRaycastingShape->getAabb(trans, min, max);
        else if (shape->mCollisionShape)
            shape->mCollisionShape->getAabb(trans, min, max);
        else
        {
            min = btVector3(0,0,0);
            max = btVector3(0,0,0);
        }
    }

    bool PhysicEngine::isAnyActorStandingOn (const std::string& objectName)
    {
        for (PhysicActorContainer::iterator it = mActorMap.begin(); it != mActorMap.end(); ++it)
        {
            if (!it->second->getOnGround())
                continue;
            Ogre::Vector3 pos = it->second->getPosition();
            btVector3 from (pos.x, pos.y, pos.z);
            btVector3 to = from - btVector3(0,0,5);
            std::pair<std::string, float> result = rayTest(from, to);
            if (result.first == objectName)
                return true;
        }
        return false;
    }

}
}
