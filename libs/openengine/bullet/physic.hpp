#ifndef OENGINE_BULLET_PHYSIC_H
#define OENGINE_BULLET_PHYSIC_H

#include <BulletDynamics/Dynamics/btRigidBody.h>
#include "BulletCollision/CollisionDispatch/btGhostObject.h"
#include <string>
#include <list>
#include <map>
#include "BulletShapeLoader.h"
#include "BulletCollision/CollisionShapes/btScaledBvhTriangleMeshShape.h"
#include <boost/shared_ptr.hpp>

class btRigidBody;
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
    class SceneManager;
}

namespace MWWorld
{
    class World;
}


namespace OEngine {
namespace Physic
{
    struct PhysicEvent;
    class PhysicEngine;
    class RigidBody;

    enum CollisionType {
        CollisionType_Nothing = 0, //<Collide with nothing
        CollisionType_World = 1<<0, //<Collide with world objects
        CollisionType_Actor = 1<<1, //<Collide sith actors
        CollisionType_HeightMap = 1<<2, //<collide with heightmap
        CollisionType_Raycasting = 1<<3,
        CollisionType_Projectile = 1<<4,
        CollisionType_Water = 1<<5
    };

    /**
     *This class is just an extension of normal btRigidBody in order to add extra info.
     *When bullet give back a btRigidBody, you can just do a static_cast to RigidBody,
     *so one never should use btRigidBody directly!
     */
    class RigidBody: public btRigidBody
    {
    public:
        RigidBody(btRigidBody::btRigidBodyConstructionInfo& CI,std::string name);
        virtual ~RigidBody();
        std::string mName;

        // Hack: placeable objects (that can be picked up by the player) have different collision behaviour.
        // This variable needs to be passed to BulletNifLoader.
        bool mPlaceable;
    };


    class PhysicActor
    {
    public:
        PhysicActor(const std::string &name, const std::string &mesh, PhysicEngine *engine, const Ogre::Vector3 &position, const Ogre::Quaternion &rotation, float scale);

        ~PhysicActor();

        void setPosition(const Ogre::Vector3 &pos);

        /**
         * Sets the collisionMode for this actor. If disabled, the actor can fly and clip geometry.
         */
        void enableCollisionMode(bool collision);

        /**
         * Enables or disables the *external* collision body. If disabled, other actors will not collide with this actor.
         */
        void enableCollisionBody(bool collision);

        bool getCollisionMode() const
        {
            return mInternalCollisionMode;
        }

        /**
         * Sets the scale of the PhysicActor
         */
        void setScale(float scale);

        void setRotation (const Ogre::Quaternion& rotation);

        const Ogre::Vector3& getPosition() const;

        /**
         * Returns the (scaled) half extents
         */
        Ogre::Vector3 getHalfExtents() const;

        /**
         * Sets the current amount of inertial force (incl. gravity) affecting this physic actor
         */
        void setInertialForce(const Ogre::Vector3 &force);

        /**
         * Gets the current amount of inertial force (incl. gravity) affecting this physic actor
         */
        const Ogre::Vector3 &getInertialForce() const
        {
            return mForce;
        }

        void setOnGround(bool grounded);

        bool getOnGround() const
        {
            return mInternalCollisionMode && mOnGround;
        }

        btCollisionObject *getCollisionBody() const
        {
            return mBody;
        }


        /// Sets whether this actor should be able to collide with the water surface
        void setCanWaterWalk(bool waterWalk);

        /// Sets whether this actor has been walking on the water surface in the last frame
        void setWalkingOnWater(bool walkingOnWater);
        bool isWalkingOnWater() const;

    private:
        /// Removes then re-adds the collision body to the dynamics world
        void updateCollisionMask();

        bool mCanWaterWalk;
        bool mWalkingOnWater;

        boost::shared_ptr<btCollisionShape> mShape;

        OEngine::Physic::RigidBody* mBody;

        Ogre::Quaternion mMeshOrientation;
        Ogre::Vector3 mMeshTranslation;
        Ogre::Vector3 mHalfExtents;

        float mScale;
        Ogre::Vector3 mPosition;

        Ogre::Vector3 mForce;
        bool mOnGround;
        bool mInternalCollisionMode;
        bool mExternalCollisionMode;

        std::string mMesh;
        std::string mName;
        PhysicEngine *mEngine;

        PhysicActor(const PhysicActor&);
        PhysicActor& operator=(const PhysicActor&);
    };


    struct HeightField
    {
        btHeightfieldTerrainShape* mShape;
        RigidBody* mBody;
    };

    struct AnimatedShapeInstance
    {
        btCollisionShape* mCompound;

        // Maps node record index to child index in the compound shape
        std::map<int, int> mAnimatedShapes;
    };

    /**
     * The PhysicEngine class contain everything which is needed for Physic.
     * It's needed that Ogre Resources are set up before the PhysicEngine is created.
     * Note:deleting it WILL NOT delete the RigidBody!
     * TODO:unload unused resources?
     */
    class PhysicEngine
    {
    public:
        /**
         * Note that the shapeLoader IS destroyed by the phyic Engine!!
         */
        PhysicEngine(BulletShapeLoader* shapeLoader);

        /**
         * It DOES destroy the shape loader!
         */
        ~PhysicEngine();

        /**
         * Creates a RigidBody.  It does not add it to the simulation.
         * After created, the body is set to the correct rotation, position, and scale
         */
        RigidBody* createAndAdjustRigidBody(const std::string &mesh, const std::string &name,
            float scale, const Ogre::Vector3 &position, const Ogre::Quaternion &rotation,
            Ogre::Vector3* scaledBoxTranslation = 0, Ogre::Quaternion* boxRotation = 0, bool raycasting=false, bool placeable=false);

        /**
         * Adjusts a rigid body to the right position and rotation
         */

        void adjustRigidBody(RigidBody* body, const Ogre::Vector3 &position, const Ogre::Quaternion &rotation,
            const Ogre::Vector3 &scaledBoxTranslation = Ogre::Vector3::ZERO,
            const Ogre::Quaternion &boxRotation = Ogre::Quaternion::IDENTITY);
        /**
         Mainly used to (but not limited to) adjust rigid bodies based on box shapes to the right position and rotation.
         */
        void boxAdjustExternal(const std::string &mesh, RigidBody* body, float scale, const Ogre::Vector3 &position, const Ogre::Quaternion &rotation);
        /**
         * Add a HeightField to the simulation
         */
        void addHeightField(float* heights,
                int x, int y, float yoffset,
                float triSize, float sqrtVerts);

        /**
         * Remove a HeightField from the simulation
         */
        void removeHeightField(int x, int y);

        /**
         * Remove a RigidBody from the simulation. It does not delete it, and does not remove it from the RigidBodyMap.
         */
        void removeRigidBody(const std::string &name);

        /**
         * Delete a RigidBody, and remove it from RigidBodyMap.
         */
        void deleteRigidBody(const std::string &name);

        /**
         * Return a pointer to a given rigid body.
         */
        RigidBody* getRigidBody(const std::string &name, bool raycasting=false);

        /**
         * Create and add a character to the scene, and add it to the ActorMap.
         */
        void addCharacter(const std::string &name, const std::string &mesh,
        const Ogre::Vector3 &position, float scale, const Ogre::Quaternion &rotation);

        /**
         * Remove a character from the scene.
         */
        void removeCharacter(const std::string &name);

        /**
         * Return a pointer to a character
         */
        PhysicActor* getCharacter(const std::string &name);

        /**
         * This step the simulation of a given time.
         */
        void stepSimulation(double deltaT);

        /**
         * Create a debug rendering. It is called by setDebgRenderingMode if it's not created yet.
         * Important Note: this will crash if the Render is not yet initialise!
         */
        void createDebugRendering();

        /**
         * Set the debug rendering mode. 0 to turn it off.
         * Important Note: this will crash if the Render is not yet initialise!
         */
        void setDebugRenderingMode(bool isDebug);

        bool toggleDebugRendering();

        void getObjectAABB(const std::string &mesh, float scale, btVector3 &min, btVector3 &max);

        void setSceneManager(Ogre::SceneManager* sceneMgr);

        /**
         * Return the closest object hit by a ray. If there are no objects, it will return ("",-1).
         * If \a normal is non-NULL, the hit normal will be written there (if there is a hit)
         */
        std::pair<std::string,float> rayTest(const btVector3& from,const btVector3& to,bool raycastingObjectOnly = true,
                                             bool ignoreHeightMap = false, Ogre::Vector3* normal = NULL);

        /**
         * Return all objects hit by a ray.
         */
        std::vector< std::pair<float, std::string> > rayTest2(const btVector3 &from, const btVector3 &to, int filterGroup=0xff);

        std::pair<bool, float> sphereCast (float radius, btVector3& from, btVector3& to);
        ///< @return (hit, relative distance)

        std::vector<std::string> getCollisions(const std::string& name, int collisionGroup, int collisionMask);

        // Get the nearest object that's inside the given object, filtering out objects of the
        // provided name
        std::pair<const RigidBody*,btVector3> getFilteredContact(const std::string &filter,
                                                                 const btVector3 &origin,
                                                                 btCollisionObject *object);

        //Bullet Stuff
        btBroadphaseInterface* broadphase;
        btDefaultCollisionConfiguration* collisionConfiguration;
        btSequentialImpulseConstraintSolver* solver;
        btCollisionDispatcher* dispatcher;
        btDiscreteDynamicsWorld* mDynamicsWorld;

        //the NIF file loader.
        BulletShapeLoader* mShapeLoader;

        typedef std::map<std::string, HeightField> HeightFieldContainer;
        HeightFieldContainer mHeightFieldMap;

        typedef std::map<std::string,RigidBody*> RigidBodyContainer;
        RigidBodyContainer mCollisionObjectMap;

        // Compound shapes that must be animated each frame based on bone positions
        // the index refers to an element in mCollisionObjectMap
        std::map<RigidBody*, AnimatedShapeInstance > mAnimatedShapes;

        RigidBodyContainer mRaycastingObjectMap;

        std::map<RigidBody*, AnimatedShapeInstance > mAnimatedRaycastingShapes;

        typedef std::map<std::string, PhysicActor*>  PhysicActorContainer;
        PhysicActorContainer mActorMap;

        Ogre::SceneManager* mSceneMgr;

        //debug rendering
        BtOgre::DebugDrawer* mDebugDrawer;
        bool isDebugCreated;
        bool mDebugActive;

        // for OpenCS with multiple engines per document
        std::map<Ogre::SceneManager *, BtOgre::DebugDrawer *> mDebugDrawers;
        std::map<Ogre::SceneManager *, Ogre::SceneNode *> mDebugSceneNodes;

        int toggleDebugRendering(Ogre::SceneManager *sceneMgr);
        void stepDebug(Ogre::SceneManager *sceneMgr);
        void createDebugDraw(Ogre::SceneManager *sceneMgr);
        void removeDebugDraw(Ogre::SceneManager *sceneMgr);

    private:
        PhysicEngine(const PhysicEngine&);
        PhysicEngine& operator=(const PhysicEngine&);
    };


    struct MyRayResultCallback : public btCollisionWorld::RayResultCallback
    {
        virtual btScalar addSingleResult( btCollisionWorld::LocalRayResult& rayResult, bool bNormalInWorldSpace)
        {
            results.push_back( std::make_pair(rayResult.m_hitFraction, rayResult.m_collisionObject) );
            return rayResult.m_hitFraction;
        }

        static bool cmp( const std::pair<float, std::string>& i, const std::pair<float, std::string>& j )
        {
            if( i.first > j.first ) return false;
            if( j.first > i.first ) return true;
            return false;
        }

        std::vector < std::pair<float, const btCollisionObject*> > results;
    };

}}

#endif
