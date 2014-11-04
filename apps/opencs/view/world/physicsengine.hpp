#ifndef CSV_WORLD_PHYSICSENGINE_H
#define CSV_WORLD_PHYSICSENGINE_H

//#include <vector>
#include <map>

#include <BulletDynamics/Dynamics/btRigidBody.h>
//#include "BulletCollision/CollisionDispatch/btGhostObject.h"
//#include <string>
//#include "BulletCollision/CollisionShapes/btScaledBvhTriangleMeshShape.h"
//#include <boost/shared_ptr.hpp>
#include <OgreSceneNode.h>
#include <openengine/bullet/BtOgreExtras.h> // needs Ogre::SceneNode defined

//#include <OgreVector3.h>
//#include <OgreQuaternion.h>

//class btRigidBody;
class btBroadphaseInterface;
class btDefaultCollisionConfiguration;
class btSequentialImpulseConstraintSolver;
class btCollisionDispatcher;
class btDiscreteDynamicsWorld;
class btHeightfieldTerrainShape;
//class btCollisionObject;

namespace BtOgre
{
    class DebugDrawer;
}

namespace Ogre
{
    class Vector3;
    class SceneNode;
    class SceneManager;
    class Quaternion;
}

namespace OEngine
{
    namespace Physic
    {
        class BulletShapeLoader;
    }
}

namespace CSVWorld
{

    //  enum btIDebugDraw::DebugDrawModes
    //  {
    //  DBG_NoDebug=0,
    //  DBG_DrawWireframe = 1,
    //  DBG_DrawAabb=2,
    //  DBG_DrawFeaturesText=4,
    //  DBG_DrawContactPoints=8,
    //  DBG_NoDeactivation=16,
    //  DBG_NoHelpText = 32,
    //  DBG_DrawText=64,
    //  DBG_ProfileTimings = 128,
    //  DBG_EnableSatComparison = 256,
    //  DBG_DisableBulletLCP = 512,
    //  DBG_EnableCCD = 1024,
    //  DBG_DrawConstraints = (1 << 11),
    //  DBG_DrawConstraintLimits = (1 << 12),
    //  DBG_FastWireframe = (1<<13),
    //  DBG_DrawNormals = (1<<14),
    //  DBG_MAX_DEBUG_DRAW_MODE
    //  };
    //
#if 0
    class CSVDebugDrawer : public BtOgre::DebugDrawer
    {
            BtOgre::DebugDrawer *mDebugDrawer;
            Ogre::SceneManager *mSceneMgr;
            Ogre::SceneNode *mSceneNode;
            int mDebugMode;

        public:

            CSVDebugDrawer(Ogre::SceneManager *sceneMgr, btDiscreteDynamicsWorld *dynamicsWorld);
            ~CSVDebugDrawer();

            void setDebugMode(int mode);
            bool toggleDebugRendering();
    };
#endif

    // This class is just an extension of normal btRigidBody in order to add extra info.
    // When bullet give back a btRigidBody, you can just do a static_cast to RigidBody,
    // so one never should use btRigidBody directly!
    class RigidBody: public btRigidBody
    {
            std::string mReferenceId;

        public:

            RigidBody(btRigidBody::btRigidBodyConstructionInfo &CI, const std::string &referenceId);
            virtual ~RigidBody();

            std::string getReferenceId() const { return mReferenceId; }
    };

    struct HeightField
    {
        btHeightfieldTerrainShape* mShape;
        RigidBody* mBody;
    };

    /**
     * The PhysicsEngine class contain everything which is needed for Physic.
     * It's needed that Ogre Resources are set up before the PhysicsEngine is created.
     * Note:deleting it WILL NOT delete the RigidBody!
     * TODO:unload unused resources?
     */
    class PhysicsEngine
    {
            //Bullet Stuff
            btBroadphaseInterface *mBroadphase;
            btDefaultCollisionConfiguration *mCollisionConfiguration;
            btSequentialImpulseConstraintSolver *mSolver;
            btCollisionDispatcher *mDispatcher;
            btDiscreteDynamicsWorld *mDynamicsWorld;

            //the NIF file loader.
            OEngine::Physic::BulletShapeLoader *mShapeLoader;

            typedef std::map<std::string, HeightField> HeightFieldContainer;
            HeightFieldContainer mHeightFieldMap;

            typedef std::map<std::string, RigidBody*> RigidBodyContainer;
            RigidBodyContainer mCollisionObjectMap;

            RigidBodyContainer mRaycastingObjectMap;

            std::map<Ogre::SceneManager *, BtOgre::DebugDrawer *> mDebugDrawers;
            std::map<Ogre::SceneManager *, Ogre::SceneNode *> mDebugSceneNodes;

#if 0
    // from bullet
    enum CollisionFilterGroups
    {
        DefaultFilter = 1,
        StaticFilter = 2,
        KinematicFilter = 4,
        DebrisFilter = 8,
        SensorTrigger = 16,
        CharacterFilter = 32,
        AllFilter = -1 //all bits sets: DefaultFilter | StaticFilter | KinematicFilter | DebrisFilter | SensorTrigger
     };
#endif

            enum CollisionType {
                CollisionType_Nothing = 0, //<Collide with nothing
                CollisionType_World = 1<<0, //<Collide with world objects
                CollisionType_Actor = 1<<1, //<Collide sith actors
                CollisionType_HeightMap = 1<<2, //<collide with heightmap
                CollisionType_Raycasting = 1<<3,
                CollisionType_Projectile = 1<<4,
                CollisionType_Water = 1<<5
            };

        public:

            PhysicsEngine();
            ~PhysicsEngine();

            // Creates a RigidBody.  It does not add it to the simulation.
            // After created, the body is set to the correct rotation, position, and scale
            RigidBody* createAndAdjustRigidBody(const std::string &mesh,
                    const std::string &referenceId, float scale,
                    const Ogre::Vector3 &position, const Ogre::Quaternion &rotation,
                    bool raycasting = false, bool placeable = false);

            // Adjusts a rigid body to the right position and rotation
            void adjustRigidBody(RigidBody* body,
                    const Ogre::Vector3 &position, const Ogre::Quaternion &rotation);
#if 0
            // Mainly used to (but not limited to) adjust rigid bodies based on box shapes
            // to the right position and rotation.
            void boxAdjustExternal(const std::string &mesh,
                    RigidBody* body, float scale, const Ogre::Vector3 &position,
                    const Ogre::Quaternion &rotation);
#endif

            // Add a HeightField to the simulation
            void addHeightField(float* heights,
                    int x, int y, float yoffset, float triSize, float sqrtVerts);

            // Remove a HeightField from the simulation
            void removeHeightField(int x, int y);

            // Remove a RigidBody from the simulation. It does not delete it,
            // and does not remove it from the RigidBodyMap.
            void removeRigidBody(const std::string &referenceId);

            // Delete a RigidBody, and remove it from RigidBodyMap.
            void deleteRigidBody(const std::string &referenceId);

            // Return a pointer to a given rigid body.
            RigidBody* getRigidBody(const std::string &referenceId, bool raycasting=false);

            void stepDebug(Ogre::SceneManager *sceneMgr);

            int toggleDebugRendering(Ogre::SceneManager *sceneMgr);

            void createDebugDraw(Ogre::SceneManager* sceneMgr);
            void removeDebugDraw(Ogre::SceneManager *sceneMgr);

            // Return the closest object hit by a ray. If there are no objects,
            // it will return ("",-1). If \a normal is non-NULL, the hit normal
            // will be written there (if there is a hit)
            std::pair<std::string,float> rayTest(const btVector3 &from,
                    const btVector3 &to, bool raycastingObjectOnly = true,
                    bool ignoreHeightMap = false, Ogre::Vector3* normal = NULL);

        private:

            PhysicsEngine(const PhysicsEngine&);
            PhysicsEngine& operator=(const PhysicsEngine&);

            // Create a debug rendering. It is called by setDebgRenderingMode if it's
            // not created yet.
            // Important Note: this will crash if the Render is not yet initialised!
            void createDebugRendering();

            // Set the debug rendering mode. 0 to turn it off.
            // Important Note: this will crash if the Render is not yet initialised!
            void setDebugRenderingMode(int mode);
#if 0
            void getObjectAABB(const std::string &mesh,
                    float scale, btVector3 &min, btVector3 &max);

            /**
             * Return all objects hit by a ray.
             */
            std::vector< std::pair<float, std::string> > rayTest2(const btVector3 &from, const btVector3 &to, int filterGroup=0xff);

            std::pair<bool, float> sphereCast (float radius, btVector3 &from, btVector3 &to);
            ///< @return (hit, relative distance)

            std::vector<std::string> getCollisions(const std::string &name, int collisionGroup, int collisionMask);

            // Get the nearest object that's inside the given object, filtering out objects of the
            // provided name
            std::pair<const RigidBody*,btVector3> getFilteredContact(const std::string &filter,
                                                                     const btVector3 &origin,
                                                                     btCollisionObject *object);
#endif
    };
}
#endif // CSV_WORLD_PHYSICSENGINE_H
