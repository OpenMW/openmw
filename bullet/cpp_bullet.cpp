#include "btBulletDynamicsCommon.h"

#include <iostream>

using namespace std;

class CustomOverlappingPairCallback;

// System variables
btDefaultCollisionConfiguration* g_collisionConfiguration;
btCollisionDispatcher *g_dispatcher;
//btBroadphaseInterface *g_broadphase;
btAxisSweep3 *g_broadphase;
btSequentialImpulseConstraintSolver* g_solver;
btDynamicsWorld *g_dynamicsWorld;

// Player variables
btCollisionObject* g_playerObject;
btConvexShape *g_playerShape;

// Player position. This is updated automatically by the physics
// system based on g_walkDirection and collisions. It is read by D
// code through cpp_getPlayerPos().
btVector3 g_playerPosition;

// Walking vector - defines direction and speed that the player
// intends to move right now. This is updated from D code each frame
// through cpp_setPlayerDir(), based on player input (and later, AI
// decisions.) The units of the vector are points per second.
btVector3 g_walkDirection;

// These variables and the class below are used in player collision
// detection. The callback is injected into the broadphase and keeps a
// continuously updated list of what objects are colliding with the
// player (in g_pairCache). This list is used in the function called
// recoverFromPenetration() below.
btHashedOverlappingPairCache* g_pairCache;
CustomOverlappingPairCallback *g_customPairCallback;

// Include the player physics
#include "cpp_player.cpp"

class CustomOverlappingPairCallback : public btOverlappingPairCallback
{
public:
  virtual btBroadphasePair* addOverlappingPair(btBroadphaseProxy* proxy0,
                                               btBroadphaseProxy* proxy1)
  {
    if (proxy0->m_clientObject==g_playerObject ||
        proxy1->m_clientObject==g_playerObject)
      return g_pairCache->addOverlappingPair(proxy0,proxy1);
    return 0;
  }

  virtual void* removeOverlappingPair(btBroadphaseProxy* proxy0,
                                      btBroadphaseProxy* proxy1,
                                      btDispatcher* dispatcher)
  {
    if (proxy0->m_clientObject==g_playerObject ||
        proxy1->m_clientObject==g_playerObject)
      return g_pairCache->removeOverlappingPair(proxy0,proxy1,dispatcher);

    return 0;
  }

  void removeOverlappingPairsContainingProxy(btBroadphaseProxy* proxy0,
                                             btDispatcher* dispatcher)
  {    if (proxy0->m_clientObject==g_playerObject)
      g_pairCache->removeOverlappingPairsContainingProxy(proxy0,dispatcher);
  }

  /* KILLME
  btBroadphasePairArray& getOverlappingPairArray()
  { return g_pairCache->getOverlappingPairArray(); }
  btOverlappingPairCache* getOverlappingPairCache()
  { return g_pairCache; }
  */
};

extern "C" int32_t cpp_initBullet()
{
  // ------- SET UP THE WORLD -------

  // Set up basic objects
  g_collisionConfiguration = new btDefaultCollisionConfiguration();
  g_dispatcher = new btCollisionDispatcher(g_collisionConfiguration);
  //g_broadphase = new btDbvtBroadphase();
  g_solver = new btSequentialImpulseConstraintSolver;

  // TODO: Figure out what to do with this. We need the user callback
  // function used below (I think), but this is only offered by this
  // broadphase implementation (as far as I can see.)
  btVector3 worldMin(-100000,-100000,-100000);
  btVector3 worldMax(100000,100000,100000);
  g_broadphase = new btAxisSweep3(worldMin,worldMax);

  g_dynamicsWorld =
    new btDiscreteDynamicsWorld(g_dispatcher,
                                g_broadphase,
                                g_solver,
                                g_collisionConfiguration);

  //g_dynamicsWorld->setGravity(btVector3(0,-10,0));


  // ------- SET UP THE PLAYER -------

  // Create the player collision shape.
  float width = 50;
  //float height = 50;
  /*
  // One possible shape is the convex hull around two spheres
  btVector3 spherePositions[2];
  btScalar sphereRadii[2];
  sphereRadii[0] = width;
  sphereRadii[1] = width;
  spherePositions[0] = btVector3 (0.0, height/2.0, 0.0);
  spherePositions[1] = btVector3 (0.0, -height/2.0, 0.0);
  m_shape = new btMultiSphereShape(btVector3(width/2.0, height/2.0, width/2.0),
                                   &spherePositions[0], &sphereRadii[0], 2);
  */
  //g_playerShape = new btCylinderShape(btVector3(50, 50, 50));
  g_playerShape = new btSphereShape(width);

  // Create the collision object
  g_playerObject = new btCollisionObject ();
  g_playerObject->setCollisionShape (g_playerShape);
  g_playerObject->setCollisionFlags (btCollisionObject::CF_NO_CONTACT_RESPONSE);


  // ------- OTHER STUFF -------

  // Create a custom callback to pick out all the objects colliding
  // with the player. We use this in the collision recovery phase.
  g_pairCache = new btHashedOverlappingPairCache();
  g_customPairCallback = new CustomOverlappingPairCallback();
  g_broadphase->setOverlappingPairUserCallback(g_customPairCallback);

  // Set up the callback that moves the player at the end of each
  // simulation step.
  g_dynamicsWorld->setInternalTickCallback(playerStepCallback);

  // Add the character collision object to the world.
  g_dynamicsWorld->addCollisionObject(g_playerObject
                                      ,btBroadphaseProxy::DebrisFilter
                                      //,btBroadphaseProxy::StaticFilter
                                      );

  // Success!
  return 0;
}

// Warp the player to a specific location. We do not bother setting
// rotation, since it's completely irrelevant for collision detection.
extern "C" void cpp_movePlayer(float x, float y, float z)
{
  btTransform tr;
  tr.setIdentity();
  tr.setOrigin(btVector3(x,y,z));
  g_playerObject->setWorldTransform(tr);
}

// Request that the player moves in this direction
extern "C" void cpp_setPlayerDir(float x, float y, float z)
{ g_walkDirection.setValue(x,y,z); }

// Get the current player position, after physics and collision have
// been applied.
extern "C" void cpp_getPlayerPos(float *x, float *y, float *z)
{
  *x = g_playerPosition.getX();
  *y = g_playerPosition.getY();
  *z = g_playerPosition.getZ();
}

// Insert a debug collision shape
extern "C" void cpp_insertBox(float x, float y, float z)
{
  btCollisionShape* groundShape =
    new btSphereShape(50);
    //new btBoxShape(btVector3(100,100,100));

  // Create a motion state to hold the object (not sure why yet.)
  btTransform groundTransform;
  groundTransform.setIdentity();
  groundTransform.setOrigin(btVector3(x,y,z));
  btDefaultMotionState* myMotionState =
    new btDefaultMotionState(groundTransform);

  // Create a rigid body from the motion state. Give it zero mass and
  // inertia.
  btRigidBody::btRigidBodyConstructionInfo
    rbInfo(0, myMotionState, groundShape, btVector3(0,0,0));
  btRigidBody* body = new btRigidBody(rbInfo);

  // Add the body to the world
  g_dynamicsWorld->addRigidBody(body);
}

// Move the physics simulation 'delta' seconds forward in time
extern "C" void cpp_timeStep(float delta)
{
  // TODO: We might experiment with the number of time steps. Remember
  // that the function also returns the number of steps performed.
  g_dynamicsWorld->stepSimulation(delta,2);
}

// Cleanup in the reverse order of creation/initialization
extern "C" void cpp_cleanupBullet()
{
  // Remove the rigidbodies from the dynamics world and delete them
  for (int i=g_dynamicsWorld->getNumCollisionObjects()-1; i>=0 ;i--)
    {
      btCollisionObject* obj = g_dynamicsWorld->getCollisionObjectArray()[i];
      btRigidBody* body = btRigidBody::upcast(obj);

      if (body && body->getMotionState())
        delete body->getMotionState();

      g_dynamicsWorld->removeCollisionObject( obj );
      delete obj;
    }

  delete g_dynamicsWorld;
  delete g_solver;
  delete g_broadphase;
  delete g_dispatcher;
  delete g_collisionConfiguration;
}
