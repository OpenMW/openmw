#include "physic.hpp"
#include <btBulletDynamicsCommon.h>
#include <btBulletCollisionCommon.h>
#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>
#include "pmove.h"
#include <components/nifbullet/bullet_nif_loader.hpp>
#include "CMotionState.h"
#include "OgreRoot.h"
#include "btKinematicCharacterController.h"
#include "BtOgrePG.h"
#include "BtOgreGP.h"
#include "BtOgreExtras.h"

#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>

#define BIT(x) (1<<(x))

namespace OEngine {
namespace Physic
{
    enum collisiontypes {
        COL_NOTHING = 0, //<Collide with nothing
        COL_WORLD = BIT(0), //<Collide with world objects
        COL_ACTOR_INTERNAL = BIT(1), //<Collide internal capsule
        COL_ACTOR_EXTERNAL = BIT(2), //<collide with external capsule
        COL_RAYCASTING = BIT(3)
    };

    PhysicActor::PhysicActor(std::string name, std::string mesh, PhysicEngine* engine, Ogre::Vector3 position, Ogre::Quaternion rotation, float scale): 
        mName(name), mEngine(engine), mMesh(mesh), mBoxScaledTranslation(0,0,0), mBoxRotationInverse(0,0,0,0), mBody(0), collisionMode(false), mBoxRotation(0,0,0,0)
    {
        mBody = mEngine->createAndAdjustRigidBody(mMesh, mName, scale, position, rotation, &mBoxScaledTranslation, &mBoxRotation);
        Ogre::Quaternion inverse = mBoxRotation.Inverse();
        mBoxRotationInverse = btQuaternion(inverse.x, inverse.y, inverse.z,inverse.w);
        mEngine->addRigidBody(mBody, false);  //Add rigid body to dynamics world, but do not add to object map
        pmove = new playerMove;
        pmove->mEngine = mEngine;
        btBoxShape* box = static_cast<btBoxShape*> (mBody->getCollisionShape());
        if(box != NULL){
            btVector3 size = box->getHalfExtentsWithMargin();
            Ogre::Vector3 halfExtents = Ogre::Vector3(size.getX(), size.getY(), size.getZ());
            pmove->ps.halfExtents = halfExtents;
        }
    }

    PhysicActor::~PhysicActor()
    {
        if(mBody){
            mEngine->dynamicsWorld->removeRigidBody(mBody);
            delete mBody;
        }
        delete pmove;
    }

    void PhysicActor::setCurrentWater(bool hasWater, int waterHeight){
        pmove->hasWater = hasWater;
        if(hasWater){
            pmove->waterHeight = waterHeight;
        }
    }

    void PhysicActor::setGravity(float gravity)
    {
        pmove->ps.gravity = gravity;
    }
    
    void PhysicActor::setSpeed(float speed)
    {
        pmove->ps.speed = speed;
    }

    void PhysicActor::enableCollisions(bool collision)
    {
        collisionMode = collision;
        if(collisionMode)
            pmove->ps.move_type=PM_NORMAL;
        else
            pmove->ps.move_type=PM_NOCLIP;
    }

    void PhysicActor::setJumpVelocity(float velocity)
    {
        pmove->ps.jump_velocity = velocity;
    }

    bool PhysicActor::getCollisionMode()
    {
        return collisionMode;
    }

    void PhysicActor::setMovement(signed char rightmove, signed char forwardmove, signed char upmove)
    {
        playerMove::playercmd& pm_ref = pmove->cmd;
        pm_ref.rightmove = rightmove;
        pm_ref.forwardmove = forwardmove;
        pm_ref.upmove = upmove;
    }

    void PhysicActor::setPmoveViewAngles(float pitch, float yaw, float roll){
        pmove->ps.viewangles.x = pitch;
        pmove->ps.viewangles.y = yaw;
        pmove->ps.viewangles.z = roll;
    }

    

    void PhysicActor::setRotation(const Ogre::Quaternion quat)
    {
        if(!quat.equals(getRotation(), Ogre::Radian(0))){
            mEngine->adjustRigidBody(mBody, getPosition(), quat, mBoxScaledTranslation, mBoxRotation);
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

    void PhysicActor::setPosition(const Ogre::Vector3 pos)
    {
        mEngine->adjustRigidBody(mBody, pos, getRotation(), mBoxScaledTranslation, mBoxRotation);
        btVector3 vec = mBody->getWorldTransform().getOrigin();
        pmove->ps.origin = Ogre::Vector3(vec.getX(), vec.getY(), vec.getZ());
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
        btBoxShape* box = static_cast<btBoxShape*> (mBody->getCollisionShape());
        if(box != NULL){
            btVector3 size = box->getHalfExtentsWithMargin();
            Ogre::Vector3 halfExtents = Ogre::Vector3(size.getX(), size.getY(), size.getZ());
            pmove->ps.halfExtents = halfExtents;
        }
    }

    void PhysicActor::runPmove(){
        Pmove(pmove);
        Ogre::Vector3 newpos = pmove->ps.origin;
        mBody->getWorldTransform().setOrigin(btVector3(newpos.x, newpos.y, newpos.z));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////


    RigidBody::RigidBody(btRigidBody::btRigidBodyConstructionInfo& CI,std::string name)
        : btRigidBody(CI)
        , mName(name)
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
            Ogre::SceneManagerEnumerator::SceneManagerIterator iter = Ogre::Root::getSingleton().getSceneManagerIterator();
            iter.begin();
            Ogre::SceneManager* scn = iter.getNext();
            Ogre::SceneNode* node = scn->getRootSceneNode()->createChildSceneNode();
            node->pitch(Ogre::Degree(-90));
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

    PhysicEngine::~PhysicEngine()
    {

        HeightFieldContainer::iterator hf_it = mHeightFieldMap.begin();
        for (; hf_it != mHeightFieldMap.end(); ++hf_it)
        {
            dynamicsWorld->removeRigidBody(hf_it->second.mBody);
            delete hf_it->second.mShape;
            delete hf_it->second.mBody;
        }

        RigidBodyContainer::iterator rb_it = ObjectMap.begin();
        for (; rb_it != ObjectMap.end(); ++rb_it)
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
    }

    void PhysicEngine::addHeightField(float* heights,
        int x, int y, float yoffset,
        float triSize, float sqrtVerts)
    {
        const std::string name = "HeightField_"
            + boost::lexical_cast<std::string>(x) + "_"
            + boost::lexical_cast<std::string>(y);

        // find the minimum and maximum heights (needed for bullet)
        float minh;
        float maxh;
        for (int i=0; i<sqrtVerts*sqrtVerts; ++i)
        {
            float h = heights[i];
            if (i==0)
            {
                minh = h;
                maxh = h;
            }

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
        body->collide = true;
        body->getWorldTransform().setOrigin(btVector3( (x+0.5)*triSize*(sqrtVerts-1), (y+0.5)*triSize*(sqrtVerts-1), (maxh+minh)/2.f));

        HeightField hf;
        hf.mBody = body;
        hf.mShape = hfShape;

        mHeightFieldMap [name] = hf;

        dynamicsWorld->addRigidBody(body,COL_WORLD,COL_WORLD|COL_ACTOR_INTERNAL|COL_ACTOR_EXTERNAL);
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

    void PhysicEngine::adjustRigidBody(RigidBody* body, Ogre::Vector3 position, Ogre::Quaternion rotation, 
        Ogre::Vector3 scaledBoxTranslation, Ogre::Quaternion boxRotation){
        btTransform tr;
        rotation = rotation * boxRotation;
        Ogre::Vector3 transrot = rotation * scaledBoxTranslation;
        Ogre::Vector3 newPosition = transrot + position;
        
        tr.setOrigin(btVector3(newPosition.x, newPosition.y, newPosition.z));
        tr.setRotation(btQuaternion(rotation.x,rotation.y,rotation.z,rotation.w));
        body->setWorldTransform(tr);
    }
    void PhysicEngine::boxAdjustExternal(std::string mesh, RigidBody* body, float scale, Ogre::Vector3 position, Ogre::Quaternion rotation){
        std::string sid = (boost::format("%07.3f") % scale).str();
        std::string outputstring = mesh + sid;
        //std::cout << "The string" << outputstring << "\n";

        //get the shape from the .nif
        mShapeLoader->load(outputstring,"General");
        BulletShapeManager::getSingletonPtr()->load(outputstring,"General");
        BulletShapePtr shape = BulletShapeManager::getSingleton().getByName(outputstring,"General");

        adjustRigidBody(body, position, rotation, shape->boxTranslation * scale, shape->boxRotation);
    }

    RigidBody* PhysicEngine::createAndAdjustRigidBody(std::string mesh,std::string name,float scale, Ogre::Vector3 position, Ogre::Quaternion rotation,
        Ogre::Vector3* scaledBoxTranslation, Ogre::Quaternion* boxRotation)
    {
        std::string sid = (boost::format("%07.3f") % scale).str();
        std::string outputstring = mesh + sid;

        //get the shape from the .nif
        mShapeLoader->load(outputstring,"General");
        BulletShapeManager::getSingletonPtr()->load(outputstring,"General");
        BulletShapePtr shape = BulletShapeManager::getSingleton().getByName(outputstring,"General");
        shape->Shape->setLocalScaling( btVector3(scale,scale,scale));


        //create the motionState
        CMotionState* newMotionState = new CMotionState(this,name);

        //create the real body
        btRigidBody::btRigidBodyConstructionInfo CI = btRigidBody::btRigidBodyConstructionInfo(0,newMotionState,shape->Shape);
        RigidBody* body = new RigidBody(CI,name);
        body->collide = shape->collide;

        if(scaledBoxTranslation != 0)
            *scaledBoxTranslation = shape->boxTranslation * scale;
        if(boxRotation != 0)
            *boxRotation = shape->boxRotation;

        adjustRigidBody(body, position, rotation, shape->boxTranslation * scale, shape->boxRotation);

        return body;

    }

    void PhysicEngine::addRigidBody(RigidBody* body, bool addToMap)
    {
        if(body)
        {
            if(body->collide)
            {
                dynamicsWorld->addRigidBody(body,COL_WORLD,COL_WORLD|COL_ACTOR_INTERNAL|COL_ACTOR_EXTERNAL);
            }
            else
            {
                dynamicsWorld->addRigidBody(body,COL_RAYCASTING,COL_RAYCASTING|COL_WORLD);
            }
            body->setActivationState(DISABLE_DEACTIVATION);
            if(addToMap){
                RigidBody* oldBody = ObjectMap[body->mName];
                if (oldBody != NULL)
                {
                    dynamicsWorld->removeRigidBody(oldBody);
                    delete oldBody;
                }

                ObjectMap[body->mName] = body;
            }
        }
    }

    void PhysicEngine::removeRigidBody(std::string name)
    {
        RigidBodyContainer::iterator it = ObjectMap.find(name);
        if (it != ObjectMap.end() )
        {
            RigidBody* body = it->second;
            if(body != NULL)
            {
                // broadphase->getOverlappingPairCache()->removeOverlappingPairsContainingProxy(body->getBroadphaseProxy(),dispatcher);
                /*PhysicActorContainer::iterator it2 = PhysicActorMap.begin();
                  for(;it2!=PhysicActorMap.end();it++)
                  {
                  it2->second->internalGhostObject->getOverlappingPairCache()->removeOverlappingPairsContainingProxy(body->getBroadphaseProxy(),dispatcher);
                  it2->second->externalGhostObject->getOverlappingPairCache()->removeOverlappingPairsContainingProxy(body->getBroadphaseProxy(),dispatcher);
                  }*/
                dynamicsWorld->removeRigidBody(body);
            }
        }
    }

    void PhysicEngine::deleteRigidBody(std::string name)
    {
        RigidBodyContainer::iterator it = ObjectMap.find(name);
        if (it != ObjectMap.end() )
        {
            RigidBody* body = it->second;
            //btScaledBvhTriangleMeshShape* scaled = dynamic_cast<btScaledBvhTriangleMeshShape*> (body->getCollisionShape());
            
            if(body != NULL)
            {
                delete body;
            }
            /*if(scaled != NULL)
            {
                delete scaled;
            }*/
            ObjectMap.erase(it);
        }
    }

    RigidBody* PhysicEngine::getRigidBody(std::string name)
    {
        RigidBodyContainer::iterator it = ObjectMap.find(name);
        if (it != ObjectMap.end() )
        {
            RigidBody* body = ObjectMap[name];
            return body;
        }
        else
        {
            return 0;
        }
    }

    void PhysicEngine::stepSimulation(double deltaT)
    {
        dynamicsWorld->stepSimulation(deltaT,10, 1/60.0);
        if(isDebugCreated)
        {
            mDebugDrawer->step();
        }
    }

    void PhysicEngine::addCharacter(std::string name, std::string mesh,
        Ogre::Vector3 position, float scale, Ogre::Quaternion rotation)
    {
        // Remove character with given name, so we don't make memory
        // leak when character would be added twice
        removeCharacter(name);

        PhysicActor* newActor = new PhysicActor(name, mesh, this, position, rotation, scale);
        

        //dynamicsWorld->addAction( newActor->mCharacter );
        PhysicActorMap[name] = newActor;
    }

    void PhysicEngine::removeCharacter(std::string name)
    {
        //std::cout << "remove";
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

    PhysicActor* PhysicEngine::getCharacter(std::string name)
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
        resultCallback1.m_collisionFilterMask = COL_WORLD|COL_RAYCASTING;
        dynamicsWorld->rayTest(from, to, resultCallback1);
        if (resultCallback1.hasHit())
        {
            name = static_cast<const RigidBody&>(*resultCallback1.m_collisionObject).mName;
            d1 = resultCallback1.m_closestHitFraction;
            d = d1;
        }

        btCollisionWorld::ClosestRayResultCallback resultCallback2(from, to);
        resultCallback2.m_collisionFilterMask = COL_ACTOR_INTERNAL|COL_ACTOR_EXTERNAL;
        dynamicsWorld->rayTest(from, to, resultCallback2);
        float d2 = 10000.;
        if (resultCallback2.hasHit())
        {
            d2 = resultCallback1.m_closestHitFraction;
            if(d2<=d1)
            {
                name = static_cast<const PairCachingGhostObject&>(*resultCallback2.m_collisionObject).mName;
                d = d2;
            }
        }

        return std::pair<std::string,float>(name,d);
    }

    std::vector< std::pair<float, std::string> > PhysicEngine::rayTest2(btVector3& from, btVector3& to)
    {
        MyRayResultCallback resultCallback1;
        resultCallback1.m_collisionFilterMask = COL_WORLD|COL_RAYCASTING;
        dynamicsWorld->rayTest(from, to, resultCallback1);
        std::vector< std::pair<float, const btCollisionObject*> > results = resultCallback1.results;

        MyRayResultCallback resultCallback2;
        resultCallback2.m_collisionFilterMask = COL_ACTOR_INTERNAL|COL_ACTOR_EXTERNAL;
        dynamicsWorld->rayTest(from, to, resultCallback2);
        std::vector< std::pair<float, const btCollisionObject*> > actorResults = resultCallback2.results;

        std::vector< std::pair<float, std::string> > results2;

        for (std::vector< std::pair<float, const btCollisionObject*> >::iterator it=results.begin();
            it != results.end(); ++it)
        {
            results2.push_back( std::make_pair( (*it).first, static_cast<const RigidBody&>(*(*it).second).mName ) );
        }

        for (std::vector< std::pair<float, const btCollisionObject*> >::iterator it=actorResults.begin();
            it != actorResults.end(); ++it)
        {
            results2.push_back( std::make_pair( (*it).first, static_cast<const PairCachingGhostObject&>(*(*it).second).mName ) );
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

        shape->Shape->getAabb(trans, min, max);
    }
}};
