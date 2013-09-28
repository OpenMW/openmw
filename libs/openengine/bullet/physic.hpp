#ifndef OENGINE_BULLET_PHYSIC_H
#define OENGINE_BULLET_PHYSIC_H

#include <BulletDynamics/Dynamics/btRigidBody.h>
#include "BulletCollision/CollisionDispatch/btGhostObject.h"
#include <string>
#include <list>
#include <map>
#include "BulletShapeLoader.h"
#include "BulletCollision/CollisionShapes/btScaledBvhTriangleMeshShape.h"



class btRigidBody;
class btBroadphaseInterface;
class btDefaultCollisionConfiguration;
class btSequentialImpulseConstraintSolver;
class btCollisionDispatcher;
class btDiscreteDynamicsWorld;
class btHeightfieldTerrainShape;
struct playerMove;

namespace BtOgre
{
    class DebugDrawer;
}

namespace MWWorld
{
    class World;
}


namespace OEngine {
namespace Physic
{
    class CMotionState;
    struct PhysicEvent;
    class PhysicEngine;
    class RigidBody;

    /**
    *This is just used to be able to name objects.
    */
    class PairCachingGhostObject : public btPairCachingGhostObject
    {
    public:
        PairCachingGhostObject(std::string name)
            :btPairCachingGhostObject(),mName(name)
        {
        }
        virtual ~PairCachingGhostObject(){}

        std::string mName;
    };

    /**
     * A physic actor uses a rigid body based on box shapes.
     * Pmove is used to move the physic actor around the dynamic world.
     */
    class PhysicActor
    {
    public:
        PhysicActor(std::string name, std::string mesh, PhysicEngine *engine, Ogre::Vector3 position, Ogre::Quaternion rotation, float scale);

        ~PhysicActor();

        void setCurrentWater(bool hasWater, int waterHeight);

        /**
         * This function sets the movement keys for pmove
         */
        void setMovement(signed char rightmove, signed char forwardmove, signed char upmove);

        /**
         * This adjusts the rotation of a PhysicActor
         * If we have any problems with this (getting stuck in pmove) we should change it 
         * from setting the visual orientation to setting the orientation of the rigid body directly.
         */
        void setRotation(const Ogre::Quaternion quat);

        void setGravity(float gravity);

        void setSpeed(float speed);

        void setJumpVelocity(float velocity);

        void enableCollisions(bool collision);

        bool getCollisionMode();

        /**
         * This returns the visual position of the PhysicActor (used to position a scenenode).
         * Note - this is different from the position of the contained mBody.
         */
        Ogre::Vector3 getPosition();

        /**
         * Returns the visual orientation of the PhysicActor
         */
        Ogre::Quaternion getRotation();

        /**
         * Sets the position of mBody from a visual position input.
         * For most cases this should not be used.  We should instead let pmove move the PhysicActor around for us
         */
        void setPosition(const Ogre::Vector3 pos);

        /**
         * Sets the view angles for pmove directly.
         * Remember, add 90 for yaw.  Set roll to 0.
         */
        void setPmoveViewAngles(float pitch, float yaw, float roll);

        /**
         * Sets the scale of the PhysicActor
         */
        void setScale(float scale);

        /**
         * Runs pmove for this PhysicActor
         */
        void runPmove();

        


    private:
        
        OEngine::Physic::RigidBody* mBody;
        Ogre::Vector3 mBoxScaledTranslation;
        btQuaternion mBoxRotationInverse;
        Ogre::Quaternion mBoxRotation;
        bool collisionMode;
        std::string mMesh;
        PhysicEngine* mEngine;
        std::string mName;
        playerMove* pmove;
       
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

        //is this body used for raycasting only?
        bool collide;
    };

    struct HeightField
    {
        btHeightfieldTerrainShape* mShape;
        RigidBody* mBody;
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
        RigidBody* createAndAdjustRigidBody(std::string mesh,std::string name,float scale, Ogre::Vector3 position, Ogre::Quaternion rotation, 
            Ogre::Vector3* scaledBoxTranslation = 0, Ogre::Quaternion* boxRotation = 0);

        /**
         * Adjusts a rigid body to the right position and rotation
         */

        void adjustRigidBody(RigidBody* body, Ogre::Vector3 position, Ogre::Quaternion rotation, 
            Ogre::Vector3 scaledBoxTranslation = Ogre::Vector3::ZERO, Ogre::Quaternion boxRotation = Ogre::Quaternion::IDENTITY);
        /**
         Mainly used to (but not limited to) adjust rigid bodies based on box shapes to the right position and rotation.
         */
        void boxAdjustExternal(std::string mesh, RigidBody* body, float scale, Ogre::Vector3 position, Ogre::Quaternion rotation);
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
         * Add a RigidBody to the simulation
         */
        void addRigidBody(RigidBody* body, bool addToMap = true);

        /**
         * Remove a RigidBody from the simulation. It does not delete it, and does not remove it from the RigidBodyMap.
         */
        void removeRigidBody(std::string name);

        /**
         * Delete a RigidBody, and remove it from RigidBodyMap.
         */
        void deleteRigidBody(std::string name);

        /**
         * Return a pointer to a given rigid body.
         * TODO:check if exist
         */
        RigidBody* getRigidBody(std::string name);

        /**
         * Create and add a character to the scene, and add it to the ActorMap.
         */
        void addCharacter(std::string name, std::string mesh,
        Ogre::Vector3 position, float scale, Ogre::Quaternion rotation);

        /**
         * Remove a character from the scene. TODO:delete it! for now, a small memory leak^^ done?
         */
        void removeCharacter(std::string name);

        /**
         * Return a pointer to a character
         * TODO:check if the actor exist...
         */
        PhysicActor* getCharacter(std::string name);

        /**
         * This step the simulation of a given time.
         */
        void stepSimulation(double deltaT);

        /**
         * Empty events lists
         */
        void emptyEventLists(void);

        /**
         * Create a debug rendering. It is called by setDebgRenderingMode if it's not created yet.
         * Important Note: this will crash if the Render is not yet initialise!
         */
        void createDebugRendering();

        /**
         * Set the debug rendering mode. 0 to turn it off.
         * Important Note: this will crash if the Render is not yet initialise!
         */
        void setDebugRenderingMode(int mode);

        bool toggleDebugRendering();

        void getObjectAABB(const std::string &mesh, float scale, btVector3 &min, btVector3 &max);

        /**
         * Return the closest object hit by a ray. If there are no objects, it will return ("",-1).
         */
        std::pair<std::string,float> rayTest(btVector3& from,btVector3& to);

        /**
         * Return all objects hit by a ray.
         */
        std::vector< std::pair<float, std::string> > rayTest2(btVector3& from, btVector3& to);

        //event list of non player object
        std::list<PhysicEvent> NPEventList;

        //event list affecting the player
        std::list<PhysicEvent> PEventList;

        //Bullet Stuff
        btOverlappingPairCache* pairCache;
        btBroadphaseInterface* broadphase;
        btDefaultCollisionConfiguration* collisionConfiguration;
        btSequentialImpulseConstraintSolver* solver;
        btCollisionDispatcher* dispatcher;
        btDiscreteDynamicsWorld* dynamicsWorld;

        //the NIF file loader.
        BulletShapeLoader* mShapeLoader;

        typedef std::map<std::string, HeightField> HeightFieldContainer;
        HeightFieldContainer mHeightFieldMap;

        typedef std::map<std::string,RigidBody*> RigidBodyContainer;
        RigidBodyContainer ObjectMap;

        typedef std::map<std::string, PhysicActor*>  PhysicActorContainer;
        PhysicActorContainer PhysicActorMap;

        //debug rendering
        BtOgre::DebugDrawer* mDebugDrawer;
        bool isDebugCreated;
        bool mDebugActive;
       
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
