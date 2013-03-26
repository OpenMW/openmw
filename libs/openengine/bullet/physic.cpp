#include "physic.hpp"
#include <btBulletDynamicsCommon.h>
#include <btBulletCollisionCommon.h>
#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>
#include <components/nifbullet/bulletnifloader.hpp>
#include "CMotionState.h"
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
      , mBody(0), onGround(false), collisionMode(true), mBoxRotation(0,0,0,0), verticalForce(0.0f)
    {
        // FIXME: Force player to start in no-collision mode for now, until he spawns at a proper door marker.
        if(name == "player")
            collisionMode = false;
        mBody = mEngine->createAndAdjustRigidBody(mMesh, mName, scale, position, rotation, &mBoxScaledTranslation, &mBoxRotation);
        mRaycastingBody = mEngine->createAndAdjustRigidBody(mMesh, mName, scale, position, rotation, &mBoxScaledTranslation, &mBoxRotation, true);
        Ogre::Quaternion inverse = mBoxRotation.Inverse();
        mBoxRotationInverse = btQuaternion(inverse.x, inverse.y, inverse.z,inverse.w);
        mEngine->addRigidBody(mBody, false, mRaycastingBody);  //Add rigid body to dynamics world, but do not add to object map
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
        if(collision && !collisionMode) mBody->translate(btVector3(0,0,-1000));
        if(!collision && collisionMode) mBody->translate(btVector3(0,0,1000));
        collisionMode = collision;
    }


    void PhysicActor::setPosition(const Ogre::Vector3 &pos)
    {
        if(pos != getPosition())
        {
            mEngine->adjustRigidBody(mBody, pos, getRotation(), mBoxScaledTranslation, mBoxRotation);
            mEngine->adjustRigidBody(mRaycastingBody, pos, getRotation(), mBoxScaledTranslation, mBoxRotation);
        }
    }

    void PhysicActor::setRotation(const Ogre::Quaternion &quat)
    {
        if(!quat.equals(getRotation(), Ogre::Radian(0))){
            mEngine->adjustRigidBody(mBody, getPosition(), quat, mBoxScaledTranslation, mBoxRotation);
            mEngine->adjustRigidBody(mRaycastingBody, getPosition(), quat, mBoxScaledTranslation, mBoxRotation);

        }
    }


    Ogre::Vector3 PhysicActor::getPosition()
    {
        btVector3 vec = mBody->getWorldTransform().getOrigin();
        Ogre::Quaternion rotation = Ogre::Quaternion(mBody->getWorldTransform().getRotation().getW(), mBody->getWorldTransform().getRotation().getX(),
            mBody->getWorldTransform().getRotation().getY(), mBody->getWorldTransform().getRotation().getZ());
        Ogre::Vector3 transrot = rotation * mBoxScaledTranslation;
        Ogre::Vector3 visualPosition = Ogre::Vector3(vec.getX(), vec.getY(), vec.getZ()) - transrot;
        return visualPosition;
    }

    Ogre::Quaternion PhysicActor::getRotation()
    {
        btQuaternion quat = mBody->getWorldTransform().getRotation() * mBoxRotationInverse;
        return Ogre::Quaternion(quat.getW(), quat.getX(), quat.getY(), quat.getZ());
    }

    void PhysicActor::setScale(float scale){
        Ogre::Vector3 position = getPosition();
        Ogre::Quaternion rotation = getRotation();
        //We only need to change the scaled box translation, box rotations remain the same.
        mBoxScaledTranslation = mBoxScaledTranslation / mBody->getCollisionShape()->getLocalScaling().getX();
        mBoxScaledTranslation *= scale;
        if(mBody){
            mEngine->dynamicsWorld->removeRigidBody(mBody);
            delete mBody;
        }
        //Create the newly scaled rigid body
        mBody = mEngine->createAndAdjustRigidBody(mMesh, mName, scale, position, rotation);
        mEngine->addRigidBody(mBody, false);  //Add rigid body to dynamics world, but do not add to object map
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

    void PhysicActor::setVerticalForce(float force)
    {
        verticalForce = force;
    }

    float PhysicActor::getVerticalForce() const
    {
        return verticalForce;
    }

    void PhysicActor::setOnGround(bool grounded)
    {
        onGround = grounded;
    }

    bool PhysicActor::getOnGround() const
    {
        return collisionMode && onGround;
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

        PhysicActorContainer::iterator pa_it = PhysicActorMap.begin();
        for (; pa_it != PhysicActorMap.end(); ++pa_it)
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

        CMotionState* newMotionState = new CMotionState(this,name);

        btRigidBody::btRigidBodyConstructionInfo CI = btRigidBody::btRigidBodyConstructionInfo(0,newMotionState,hfShape);
        RigidBody* body = new RigidBody(CI,name);
        body->getWorldTransform().setOrigin(btVector3( (x+0.5)*triSize*(sqrtVerts-1), (y+0.5)*triSize*(sqrtVerts-1), (maxh+minh)/2.f));

        HeightField hf;
        hf.mBody = body;
        hf.mShape = hfShape;

        mHeightFieldMap [name] = hf;

        dynamicsWorld->addRigidBody(body,CollisionType_World,CollisionType_World|CollisionType_ActorInternal|CollisionType_ActorExternal);
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

        //create the motionState
        CMotionState* newMotionState = new CMotionState(this,name);

        //create the real body
        btRigidBody::btRigidBodyConstructionInfo CI = btRigidBody::btRigidBodyConstructionInfo
                (0,newMotionState, raycasting ? shape->mRaycastingShape : shape->mCollisionShape);
        RigidBody* body = new RigidBody(CI,name);
        body->mPlaceable = placeable;

        if(scaledBoxTranslation != 0)
            *scaledBoxTranslation = shape->mBoxTranslation * scale;
        if(boxRotation != 0)
            *boxRotation = shape->mBoxRotation;

        adjustRigidBody(body, position, rotation, shape->mBoxTranslation * scale, shape->mBoxRotation);

        return body;

    }

    void PhysicEngine::addRigidBody(RigidBody* body, bool addToMap, RigidBody* raycastingBody)
    {
        if(!body && !raycastingBody)
            return; // nothing to do

        const std::string& name = (body ? body->mName : raycastingBody->mName);

        if (body)
            dynamicsWorld->addRigidBody(body,CollisionType_World,CollisionType_World|CollisionType_ActorInternal|CollisionType_ActorExternal);

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

    void PhysicEngine::stepSimulation(double deltaT)
    {
        // This isn't needed as there are no dynamic objects at this point
        //dynamicsWorld->stepSimulation(deltaT,10, 1/60.0);
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
        PhysicActorMap[name] = newActor;
    }

    void PhysicEngine::removeCharacter(const std::string &name)
    {
        PhysicActorContainer::iterator it = PhysicActorMap.find(name);
        if (it != PhysicActorMap.end() )
        {
            PhysicActor* act = it->second;
            if(act != NULL)
            {
                
                delete act;
            }
            PhysicActorMap.erase(it);
        }
    }

    PhysicActor* PhysicEngine::getCharacter(const std::string &name)
    {
        PhysicActorContainer::iterator it = PhysicActorMap.find(name);
        if (it != PhysicActorMap.end() )
        {
            PhysicActor* act = PhysicActorMap[name];
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

    std::pair<std::string,float> PhysicEngine::rayTest(btVector3& from,btVector3& to)
    {
        std::string name = "";
        float d = -1;

        float d1 = 10000.;
        btCollisionWorld::ClosestRayResultCallback resultCallback1(from, to);
        resultCallback1.m_collisionFilterMask = CollisionType_Raycasting;
        dynamicsWorld->rayTest(from, to, resultCallback1);
        if (resultCallback1.hasHit())
        {
            name = static_cast<const RigidBody&>(*resultCallback1.m_collisionObject).mName;
            d1 = resultCallback1.m_closestHitFraction;
            d = d1;
        }

        return std::pair<std::string,float>(name,d);
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
}}
