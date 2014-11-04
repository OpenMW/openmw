#include "physicsengine.hpp"

#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>

#include <BulletDynamics/btBulletDynamicsCommon.h>
#include <BulletCollision/btBulletCollisionCommon.h>
#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>

#include <OgreSceneManager.h>

#include <components/nifbullet/bulletnifloader.hpp>
#include <openengine/bullet/BulletShapeLoader.h>
#include <OgreSceneNode.h>
#include <openengine/bullet/BtOgreExtras.h> // needs Ogre::SceneNode defined

// PLEASE NOTE:
//
// This file is based on libs/openengine/bullet/physic.cpp.  Please see the commit
// history and credits for the code below, which is mostly copied from there and
// adapted for use with OpenCS.

namespace CSVWorld
{
    RigidBody::RigidBody(btRigidBody::btRigidBodyConstructionInfo &CI,
            const std::string &referenceId) : btRigidBody(CI) , mReferenceId(referenceId)
    {
    }

    RigidBody::~RigidBody()
    {
        delete getMotionState();
    }

    // In OpenCS one document has one PhysicsSystem.  Each PhysicsSystem contains one
    // PhysicsEngine.  One document can have 0..n SceneWidgets, each with its own
    // Ogre::SceneManager.
    //
    // These relationships are managed by the PhysicsManager.
    //
    //  - When a view is created its document is registered with the PhysicsManager in
    //    View's constructor.  If the document is new a PhysicSystem is created and
    //    associated with that document.  A null list of SceneWidgets are assigned to
    //    that document.
    //
    //  - When all views for a given document is closed, ViewManager will notify the
    //    PhysicsManager to destroy the PhysicsSystem associated with the document.
    //
    //  - Each time a WorldspaceWidget (or its subclass) is created, it gets the
    //    PhysicsSystem associates with the widget's document from the PhysicsManager.
    //    The list of widgets are then maintained, but is not necessary and may be
    //    removed in future code updates.
    //
    //  Each WorldspaceWidget can have objects (References) and terrain (Land) loaded
    //  from its document.  There may be several views of the object, however there
    //  is only one corresponding physics object.  i.e. each Reference can have 1..n
    //  SceneNodes
    //
    //  These relationships are managed by the PhysicsSystem for the document.
    //
    //  - Each time a WorldspaceWidget (or its subclass) is created, it registers
    //    itself and its Ogre::SceneManager with the PhysicsSystem assigned by the
    //    PhysicsManager.
    //
    //  - Each time an object is added, the object's Ogre::SceneNode name is registered
    //    with the PhysicsSystem. Ogre itself maintains which SceneNode belongs to
    //    which SceneManager.
    //
    //  - Each time an terrain is added, its cell coordinates and the SceneManager is
    //    registered with the PhysicsSystem.
    //
    PhysicsEngine::PhysicsEngine()
    {
        // Set up the collision configuration and dispatcher
        mCollisionConfiguration = new btDefaultCollisionConfiguration();
        mDispatcher = new btCollisionDispatcher(mCollisionConfiguration);

        // The actual physics solver
        mSolver = new btSequentialImpulseConstraintSolver;

        mBroadphase = new btDbvtBroadphase();

        // The world.
        mDynamicsWorld = new btDiscreteDynamicsWorld(mDispatcher,
                mBroadphase, mSolver, mCollisionConfiguration);

        // Don't update AABBs of all objects every frame. Most objects in MW are static,
        // so we don't need this.  Should a "static" object ever be moved, we have to
        // update its AABB manually using DynamicsWorld::updateSingleAabb.
        mDynamicsWorld->setForceUpdateAllAabbs(false);

        mDynamicsWorld->setGravity(btVector3(0, 0, -10));

        if(OEngine::Physic::BulletShapeManager::getSingletonPtr() == NULL)
        {
            new OEngine::Physic::BulletShapeManager();
        }

        mShapeLoader = new NifBullet::ManualBulletShapeLoader();
    }

    PhysicsEngine::~PhysicsEngine()
    {
        HeightFieldContainer::iterator hf_it = mHeightFieldMap.begin();
        for(; hf_it != mHeightFieldMap.end(); ++hf_it)
        {
            mDynamicsWorld->removeRigidBody(hf_it->second.mBody);
            delete hf_it->second.mShape;
            delete hf_it->second.mBody;
        }

        RigidBodyContainer::iterator rb_it = mCollisionObjectMap.begin();
        for(; rb_it != mCollisionObjectMap.end(); ++rb_it)
        {
            if (rb_it->second != NULL)
            {
                mDynamicsWorld->removeRigidBody(rb_it->second);

                delete rb_it->second;
                rb_it->second = NULL;
            }
        }

        rb_it = mRaycastingObjectMap.begin();
        for (; rb_it != mRaycastingObjectMap.end(); ++rb_it)
        {
            if (rb_it->second != NULL)
            {
                mDynamicsWorld->removeRigidBody(rb_it->second);

                delete rb_it->second;
                rb_it->second = NULL;
            }
        }


        delete mDynamicsWorld;
        delete mSolver;
        delete mCollisionConfiguration;
        delete mDispatcher;
        delete mBroadphase;
        delete mShapeLoader;

        // NOTE: the global resources such as "BtOgre/DebugLines" and the
        // BulletShapeManager singleton need to be deleted only when all physics
        // engines are deleted in PhysicsManager::removeDocument()
    }

    int PhysicsEngine::toggleDebugRendering(Ogre::SceneManager *sceneMgr)
    {
        if(!sceneMgr)
            return 0;

        std::map<Ogre::SceneManager *, BtOgre::DebugDrawer *>::iterator iter =
            mDebugDrawers.find(sceneMgr);
        if(iter != mDebugDrawers.end()) // found scene manager
        {
            if((*iter).second)
            {
                // set a new drawer each time (maybe with a different scene manager)
                mDynamicsWorld->setDebugDrawer(mDebugDrawers[sceneMgr]);
                if(!mDebugDrawers[sceneMgr]->getDebugMode())
                    mDebugDrawers[sceneMgr]->setDebugMode(1 /*mDebugDrawFlags*/);
                else
                    mDebugDrawers[sceneMgr]->setDebugMode(0);
                mDynamicsWorld->debugDrawWorld();

                return mDebugDrawers[sceneMgr]->getDebugMode();
            }
        }
        return 0;
    }

    void PhysicsEngine::stepDebug(Ogre::SceneManager *sceneMgr)
    {
        if(!sceneMgr)
            return;

        std::map<Ogre::SceneManager *, BtOgre::DebugDrawer *>::iterator iter =
            mDebugDrawers.find(sceneMgr);
        if(iter != mDebugDrawers.end()) // found scene manager
        {
            if((*iter).second)
                (*iter).second->step();
            else
                return;
        }
    }

    void PhysicsEngine::createDebugDraw(Ogre::SceneManager *sceneMgr)
    {
        if(mDebugDrawers.find(sceneMgr) == mDebugDrawers.end())
        {
            mDebugSceneNodes[sceneMgr] = sceneMgr->getRootSceneNode()->createChildSceneNode();
            mDebugDrawers[sceneMgr] = new BtOgre::DebugDrawer(mDebugSceneNodes[sceneMgr], mDynamicsWorld);
            mDebugDrawers[sceneMgr]->setDebugMode(0);
        }
    }

    void PhysicsEngine::removeDebugDraw(Ogre::SceneManager *sceneMgr)
    {
        std::map<Ogre::SceneManager *, BtOgre::DebugDrawer *>::iterator iter =
            mDebugDrawers.find(sceneMgr);
        if(iter != mDebugDrawers.end())
        {
            delete (*iter).second;
            mDebugDrawers.erase(iter);

            // BtOgre::DebugDrawer destroys the resources leading to crashes in some
            // situations.  Workaround by recreating them each time.
            if (!Ogre::ResourceGroupManager::getSingleton().resourceGroupExists("BtOgre"))
                Ogre::ResourceGroupManager::getSingleton().createResourceGroup("BtOgre");
            if (!Ogre::MaterialManager::getSingleton().resourceExists("BtOgre/DebugLines"))
            {
                Ogre::MaterialPtr mat = Ogre::MaterialManager::getSingleton().create("BtOgre/DebugLines", "BtOgre");
                mat->setReceiveShadows(false);
                mat->setSelfIllumination(1,1,1);
            }
        }

        std::map<Ogre::SceneManager *, Ogre::SceneNode *>::iterator it =
            mDebugSceneNodes.find(sceneMgr);
        if(it != mDebugSceneNodes.end())
        {
            std::string sceneNodeName = (*it).second->getName();
            if(sceneMgr->hasSceneNode(sceneNodeName))
                sceneMgr->destroySceneNode(sceneNodeName);
        }
    }

    void PhysicsEngine::addHeightField(float* heights,
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

        btRigidBody::btRigidBodyConstructionInfo CI =
            btRigidBody::btRigidBodyConstructionInfo(0,0,hfShape);
        RigidBody* body = new RigidBody(CI, name);
        body->getWorldTransform().setOrigin(btVector3(
                                                (x+0.5)*triSize*(sqrtVerts-1),
                                                (y+0.5)*triSize*(sqrtVerts-1),
                                                (maxh+minh)/2.f));

        HeightField hf;
        hf.mBody = body;
        hf.mShape = hfShape;

        mHeightFieldMap [name] = hf;

        mDynamicsWorld->addRigidBody(body, CollisionType_HeightMap,
                CollisionType_Actor|CollisionType_Raycasting|CollisionType_Projectile);
    }

    void PhysicsEngine::removeHeightField(int x, int y)
    {
        const std::string name = "HeightField_"
            + boost::lexical_cast<std::string>(x) + "_"
            + boost::lexical_cast<std::string>(y);

        HeightField hf = mHeightFieldMap [name];

        mDynamicsWorld->removeRigidBody(hf.mBody);
        delete hf.mShape;
        delete hf.mBody;

        mHeightFieldMap.erase(name);
    }

    void PhysicsEngine::adjustRigidBody(RigidBody* body,
            const Ogre::Vector3 &position, const Ogre::Quaternion &rotation)
    {
        btTransform tr;
        Ogre::Quaternion boxrot = rotation * Ogre::Quaternion::IDENTITY;
        Ogre::Vector3 transrot = boxrot * Ogre::Vector3::ZERO;
        Ogre::Vector3 newPosition = transrot + position;

        tr.setOrigin(btVector3(newPosition.x, newPosition.y, newPosition.z));
        tr.setRotation(btQuaternion(boxrot.x,boxrot.y,boxrot.z,boxrot.w));
        body->setWorldTransform(tr);
    }

    RigidBody* PhysicsEngine::createAndAdjustRigidBody(const std::string &mesh,
            const std::string &name, // referenceId, assumed unique per OpenCS document
            float scale,
            const Ogre::Vector3 &position,
            const Ogre::Quaternion &rotation,
            bool raycasting,
            bool placeable)          // indicates whether the object can be picked up by the player
    {
        std::string sid = (boost::format("%07.3f") % scale).str();
        std::string outputstring = mesh + sid;

        //get the shape from the .nif
        mShapeLoader->load(outputstring, "General");
        OEngine::Physic::BulletShapeManager::getSingletonPtr()->load(outputstring, "General");
        OEngine::Physic::BulletShapePtr shape =
            OEngine::Physic::BulletShapeManager::getSingleton().getByName(outputstring, "General");

        if (placeable && !raycasting && shape->mCollisionShape && !shape->mHasCollisionNode)
            return NULL;

        if (!shape->mCollisionShape && !raycasting)
            return NULL;

        if (!shape->mRaycastingShape && raycasting)
            return NULL;

        btCollisionShape* collisionShape = raycasting ? shape->mRaycastingShape : shape->mCollisionShape;
        collisionShape->setLocalScaling(btVector3(scale, scale, scale));

        //create the real body
        btRigidBody::btRigidBodyConstructionInfo CI =
            btRigidBody::btRigidBodyConstructionInfo(0, 0, collisionShape);

        RigidBody* body = new RigidBody(CI, name);

        adjustRigidBody(body, position, rotation);

        if (!raycasting)
        {
            assert (mCollisionObjectMap.find(name) == mCollisionObjectMap.end());
            mCollisionObjectMap[name] = body;
            mDynamicsWorld->addRigidBody(body,
                    CollisionType_World, CollisionType_Actor|CollisionType_HeightMap);
        }
        else
        {
            assert (mRaycastingObjectMap.find(name) == mRaycastingObjectMap.end());
            mRaycastingObjectMap[name] = body;
            mDynamicsWorld->addRigidBody(body,
                    CollisionType_Raycasting, CollisionType_Raycasting|CollisionType_Projectile);
        }

        return body;
    }

    void PhysicsEngine::removeRigidBody(const std::string &name)
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

        it = mRaycastingObjectMap.find(name);
        if (it != mRaycastingObjectMap.end() )
        {
            RigidBody* body = it->second;
            if(body != NULL)
            {
                mDynamicsWorld->removeRigidBody(body);
            }
        }
    }

    void PhysicsEngine::deleteRigidBody(const std::string &name)
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

    RigidBody* PhysicsEngine::getRigidBody(const std::string &name, bool raycasting)
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

    std::pair<std::string, float> PhysicsEngine::rayTest(const btVector3 &from,
            const btVector3 &to, bool raycastingObjectOnly, bool ignoreHeightMap, Ogre::Vector3* normal)
    {
        std::string referenceId = "";
        float d = -1;

        btCollisionWorld::ClosestRayResultCallback resultCallback1(from, to);
        resultCallback1.m_collisionFilterGroup = 0xff;

        if(raycastingObjectOnly)
            resultCallback1.m_collisionFilterMask = CollisionType_Raycasting|CollisionType_Actor;
        else
            resultCallback1.m_collisionFilterMask = CollisionType_World;

        if(!ignoreHeightMap)
            resultCallback1.m_collisionFilterMask = resultCallback1.m_collisionFilterMask | CollisionType_HeightMap;

        mDynamicsWorld->rayTest(from, to, resultCallback1);
        if (resultCallback1.hasHit())
        {
            referenceId = static_cast<const RigidBody&>(*resultCallback1.m_collisionObject).getReferenceId();
            d = resultCallback1.m_closestHitFraction;
            if (normal)
                *normal = Ogre::Vector3(resultCallback1.m_hitNormalWorld.x(),
                                        resultCallback1.m_hitNormalWorld.y(),
                                        resultCallback1.m_hitNormalWorld.z());
        }

        return std::pair<std::string, float>(referenceId, d);
    }
}
