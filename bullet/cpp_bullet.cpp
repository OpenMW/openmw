/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (cpp_bullet.cpp) is part of the OpenMW package.

  OpenMW is distributed as free software: you can redistribute it
  and/or modify it under the terms of the GNU General Public License
  version 3, as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  version 3 along with this program. If not, see
  http://www.gnu.org/licenses/ .

 */

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
// code through bullet_getPlayerPos().
btVector3 g_playerPosition;

// Walking vector - defines direction and speed that the player
// intends to move right now. This is updated from D code each frame
// through bullet_setPlayerDir(), based on player input (and later, AI
// decisions.) The units of the vector are points per second.
btVector3 g_walkDirection;

// The current trimesh shape being built. All new inserted meshes are
// added into this, until bullet_getFinalShape() is called.
btTriangleIndexVertexArray *g_currentMesh;

// These variables and the class below are used in player collision
// detection. The callback is injected into the broadphase and keeps a
// continuously updated list of what objects are colliding with the
// player (in g_pairCache). This list is used in the function called
// recoverFromPenetration().
btHashedOverlappingPairCache* g_pairCache;
CustomOverlappingPairCallback *g_customPairCallback;

// Three physics modes: walking (with gravity and collision), flying
// (collision but no gravity) and ghost mode (fly through walls)
enum
  {
    PHYS_WALK,
    PHYS_FLY,
    PHYS_GHOST
  };
int g_physMode;

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

extern "C" int32_t bullet_init()
{
  // ------- SET UP THE WORLD -------

  // Set up basic objects
  g_collisionConfiguration = new btDefaultCollisionConfiguration();
  g_dispatcher = new btCollisionDispatcher(g_collisionConfiguration);
  //g_broadphase = new btDbvtBroadphase();
  g_solver = new btSequentialImpulseConstraintSolver;

  // TODO: Figure out what to do with this. We need the user callback
  // function used below (I think), but this is only offered by this
  // broadphase implementation (as far as I can see.) Maybe we can
  // scan through the cell first and find good values that covers all
  // the objects before we set up the dynamic world. Another option is
  // to create a custom broadphase designed for our purpose. (We
  // should probably use different ones for interior and exterior
  // cells in any case.)
  btVector3 worldMin(-40000,-40000,-40000);
  btVector3 worldMax(40000,40000,40000);
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
  /*
  // One possible shape is the convex hull around two spheres
  float height = 100;
  btVector3 spherePositions[2];
  btScalar sphereRadii[2];
  sphereRadii[0] = width;
  sphereRadii[1] = width;
  spherePositions[0] = btVector3 (0.0, height/2.0, 0.0);
  spherePositions[1] = btVector3 (0.0, -height/2.0, 0.0);
  g_playerShape = new btMultiSphereShape(btVector3(width/2.0, height/2.0,
                       width/2.0), &spherePositions[0], &sphereRadii[0], 2);
  //*/
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

  // Make sure this is zero at startup
  g_currentMesh = NULL;

  // Start out walking
  g_physMode = PHYS_WALK;

  // Success!
  return 0;
}

// Switch to the next physics mode
extern "C" void bullet_nextMode()
{
  g_physMode++;
  if(g_physMode > PHYS_GHOST)
    g_physMode = PHYS_WALK;

  switch(g_physMode)
    {
    case PHYS_WALK:
      cout << "Entering walking mode\n";
      break;
    case PHYS_FLY:
      cout << "Entering flying mode\n";
      break;
    case PHYS_GHOST:
      cout << "Entering ghost mode\n";
      break;
    }
}

// Warp the player to a specific location. We do not bother setting
// rotation, since it's completely irrelevant for collision detection.
extern "C" void bullet_movePlayer(float x, float y, float z)
{
  btTransform tr;
  tr.setIdentity();
  tr.setOrigin(btVector3(x,y,z));
  g_playerObject->setWorldTransform(tr);
}

// Request that the player moves in this direction
extern "C" void bullet_setPlayerDir(float x, float y, float z)
{ g_walkDirection.setValue(x,y,z); }

// Get the current player position, after physics and collision have
// been applied.
extern "C" void bullet_getPlayerPos(float *x, float *y, float *z)
{
  *x = g_playerPosition.getX();
  *y = g_playerPosition.getY();
  *z = g_playerPosition.getZ();
}

unsigned char* copyBuffer(void *buf, int elemSize, int len)
{
  int size = elemSize * len;
  void *res = malloc(size);
  memcpy(res, buf, size);

  return (unsigned char*)res;
}

// Create a triangle shape and insert it into the current index/vertex
// array. If no array is active, create one.
extern "C" void bullet_createTriShape(int32_t numFaces,
                                      void *triArray,
                                      int32_t numVerts,
                                      void *vertArray,
                                      float *trans,
                                      float *matrix)
{
  // This struct holds the index and vertex buffers of a single
  // trimesh.
  btIndexedMesh im;

  // Set up the triangles
  int numTriangles = numFaces / 3;
  im.m_numTriangles = numTriangles;
  im.m_triangleIndexStride = 6; // 3 indices * 2 bytes per short
  im.m_triangleIndexBase = copyBuffer(triArray, 6, numTriangles);

  // Set up the vertices
  im.m_numVertices = numVerts;
  im.m_vertexStride = 12; // 4 bytes per float * 3 floats per vertex
  im.m_vertexBase = copyBuffer(vertArray, 12, numVerts);

  // Transform all the vertex values according to 'trans' and 'matrix'
  float *vb = (float*) im.m_vertexBase;
  for(int i=0; i<numVerts; i++)
    {
      float x,y,z;

      // Reinventing basic linear algebra for the win!
      x = matrix[0]*vb[0]+matrix[1]*vb[1]+matrix[2]*vb[2] + trans[0];
      y = matrix[3]*vb[0]+matrix[4]*vb[1]+matrix[5]*vb[2] + trans[1];
      z = matrix[6]*vb[0]+matrix[7]*vb[1]+matrix[8]*vb[2] + trans[2];
      *(vb++) = x;
      *(vb++) = y;
      *(vb++) = z;
    }

  // If no mesh is currently active, create one
  if(g_currentMesh == NULL)
    g_currentMesh = new btTriangleIndexVertexArray;

  // Add the mesh. Nif data stores triangle indices as shorts.
  g_currentMesh->addIndexedMesh(im, PHY_SHORT);
}

// Get the shape built up so far, if any. This clears g_currentMesh,
// so the next call to createTriShape will start a new shape.
extern "C" btCollisionShape *bullet_getFinalShape()
{
  // Return null if no meshes have been inserted
  if(g_currentMesh == NULL) return NULL;

  // Create the shape from the completed mesh
  btBvhTriangleMeshShape *shape =
    new btBvhTriangleMeshShape(g_currentMesh, false);

  g_currentMesh = NULL;
  return shape;
}

// Insert a static mesh
extern "C" void bullet_insertStatic(btCollisionShape *shape,
                                    float *pos,
                                    float *quat,
                                    float scale)
{
  // TODO: Good test case for scaled meshes: Aharunartus, some of the
  // stairs inside the cavern currently don't collide
  if(scale != 1.0)
    {
      cout << "WARNING: Cannot scale collision meshes yet (wanted "
           << scale << ")\n";
      return;
    }

  btTransform trafo;
  trafo.setIdentity();
  trafo.setOrigin(btVector3(pos[0], pos[1], pos[2]));

  // Ogre uses WXYZ quaternions, Bullet uses XYZW.
  trafo.setRotation(btQuaternion(quat[1], quat[2], quat[3], quat[0]));

  // Create and insert the collision object
  btCollisionObject *obj = new btCollisionObject();
  obj->setCollisionShape(shape);
  obj->setWorldTransform(trafo);
  g_dynamicsWorld->addCollisionObject(obj);
}


/*
// Insert a debug collision shape
extern "C" void bullet_insertBox(float x, float y, float z)
{
  btCollisionShape* groundShape =
    new btSphereShape(50);
    //new btBoxShape(btVector3(100,100,100));

  // Create a motion state to hold the object (not sure why yet.)
  btTransform groundTransform;
  groundTransform.setIdentity();
  groundTransform.setOrigin(btVector3(x,y,z));

  // Use a simple collision object for static objects
  btCollisionObject *obj = new btCollisionObject();
  obj->setCollisionShape(groundShape);
  obj->setWorldTransform(groundTransform);

  g_dynamicsWorld->addCollisionObject(obj);

  // You can also use a rigid body with a motion state, but this is
  // overkill for statics.
  /*
  btDefaultMotionState* myMotionState =
    new btDefaultMotionState(groundTransform);

  // Create a rigid body from the motion state. Give it zero mass and
  // zero inertia.
  btRigidBody::btRigidBodyConstructionInfo
    rbInfo(0, myMotionState, groundShape, btVector3(0,0,0));
  btRigidBody* body = new btRigidBody(rbInfo);

  // Add the body to the world
  g_dynamicsWorld->addRigidBody(body);
  */
//}

// Move the physics simulation 'delta' seconds forward in time
extern "C" void bullet_timeStep(float delta)
{
  // TODO: We might experiment with the number of time steps. Remember
  // that the function also returns the number of steps performed.
  g_dynamicsWorld->stepSimulation(delta,2);
}

// Cleanup in the reverse order of creation/initialization
extern "C" void bullet_cleanup()
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
