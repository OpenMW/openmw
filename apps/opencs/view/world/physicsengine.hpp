#ifndef CSV_WORLD_PHYSICSENGINE_H
#define CSV_WORLD_PHYSICSENGINE_H

#include <string>
#include <map>

#include <BulletDynamics/Dynamics/btRigidBody.h>

class btBroadphaseInterface;
class btDefaultCollisionConfiguration;
class btSequentialImpulseConstraintSolver;
class btCollisionDispatcher;
class btDiscreteDynamicsWorld;
class btHeightfieldTerrainShape;

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

    // The PhysicsEngine class contain everything which is needed for Physic.
    // It's needed that Ogre Resources are set up before the PhysicsEngine is created.
    // Note:deleting it WILL NOT delete the RigidBody!
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
    };
}
#endif // CSV_WORLD_PHYSICSENGINE_H
