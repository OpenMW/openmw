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

enum
  {
    MASK_PLAYER = 1,
    MASK_STATIC = 2
  };

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

// Current compound shape being built. g_compoundShape and
// g_currentMesh are returned together (the mesh inserted into the
// compound) if both are present.
btCompoundShape *g_compoundShape;

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

// Include the uniform shape scaler
//#include "cpp_scale.cpp"

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
  btVector3 worldMin(-20000,-20000,-20000);
  btVector3 worldMax(20000,20000,20000);
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
  float height = 50;
  btVector3 spherePositions[2];
  btScalar sphereRadii[2];
  sphereRadii[0] = width;
  sphereRadii[1] = width;
  spherePositions[0] = btVector3 (0,0,0);
  spherePositions[1] = btVector3 (0,0,-height);

  // One possible shape is the convex hull around two spheres
  g_playerShape = new btMultiSphereShape(btVector3(width/2.0, height/2.0,
                       width/2.0), &spherePositions[0], &sphereRadii[0], 2);
  */

  // Other posibilities - most are too slow, except the sphere
  //g_playerShape = new btCylinderShapeZ(btVector3(width, width, height));
  g_playerShape = new btSphereShape(width);
  //g_playerShape = new btCapsuleShapeZ(width, height);

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
  g_dynamicsWorld->addCollisionObject(g_playerObject,
                                      MASK_PLAYER,
                                      MASK_STATIC);

  // Make sure these is zero at startup
  g_currentMesh = NULL;
  g_compoundShape = NULL;

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

// Create a box shape and add it to the cumulative shape of the
// current object
/*
extern "C" void bullet_createBoxShape(float minX, float minY, float minZ,
                                      float maxX, float maxY, float maxZ,
                                      float *trans,float *matrix)
{
  if(g_compoundShape == NULL)
    g_compoundShape = new btCompoundShape;

  // Build a box from the given data.
  int x = (maxX-minX)/2;
  int y = (maxY-minY)/2;
  int z = (maxZ-minZ)/2;
  btBoxShape *box = new btBoxShape(btVector3(x,y,z));

  // Next, create the transformations
  btMatrix3x3 mat(matrix[0], matrix[1], matrix[2],
                  matrix[3], matrix[4], matrix[5],
                  matrix[6], matrix[7], matrix[8]);

  // In addition to the mesh's world translation, we have to add the
  // local translation of the box origin to fit with the given min/max
  // values.
  x += minX + trans[0];
  y += minY + trans[1];
  z += minZ + trans[2];
  btVector3 trns(x,y,z);

  // Finally, add the shape to the compound
  btTransform tr(mat, trns);
  g_compoundShape->addChildShape(tr, box);
}
*/

void* copyBuffer(void *buf, int elemSize, int len)
{
  int size = elemSize * len;
  void *res = malloc(size);
  memcpy(res, buf, size);

  return res;
}

// Internal version that does not copy buffers
void createTriShape(int32_t numFaces, void *triArray,
                    int32_t numVerts, void *vertArray,
                    float *trans, float *matrix)
{
  // This struct holds the index and vertex buffers of a single
  // trimesh.
  btIndexedMesh im;

  // Set up the triangles
  int numTriangles = numFaces / 3;
  im.m_numTriangles = numTriangles;
  im.m_triangleIndexStride = 6; // 3 indices * 2 bytes per short
  im.m_triangleIndexBase = (unsigned char*)triArray;

  // Set up the vertices
  im.m_numVertices = numVerts;
  im.m_vertexStride = 12; // 4 bytes per float * 3 floats per vertex
  im.m_vertexBase = (unsigned char*)vertArray;

  // Transform vertex values in vb according to 'trans' and 'matrix'
  float *vb = (float*)im.m_vertexBase;
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

// Define a cube with coordinates 0,0,0 - 1,1,1.
const float cube_verts[] =
  {
    0,0,0,   1,0,0,   0,1,0,
    1,1,0,   0,0,1,   1,0,1,
    0,1,1,   1,1,1
  };

// Triangles of the cube. The orientation of each triange doesn't
// matter.
const short cube_tris[] =
  {
    // bottom side
    0, 1, 2,
    1, 2, 3,
    // top side
    4, 5, 6,
    5, 6, 7,
    // front side
    0, 4, 5,
    0, 1, 5,
    // back side
    2, 3, 7,
    2, 6, 7,
    // left side
    0, 2, 4,
    2, 4, 6,
    // right side
    1, 3, 5,
    3, 5, 7
  };

const int cube_num_verts = 8;
const int cube_num_tris = 12;

// Create a (trimesh) box with the given dimensions. Used for bounding
// boxes. TODO: I guess we have to use the NIF-specified bounding box
// for this, not our automatically calculated one.
/*
extern "C" void bullet_createBox(float xmin, float ymin, float zmin,
                                 float xmax, float ymax, float zmax,
                                 float *trans, float *matrix)
{
  // Make a copy of the vertex buffer, since we need to change it
  float *vbuffer = (float*)copyBuffer(cube_verts, 12, cube_num_verts);

  // Calculate the widths
  float xwidth = xmax-xmin;
  float ywidth = ymax-ymin;
  float zwidth = zmax-zmin;

  // Transform the cube to (xmin,xmax) etc
  float *vb = vbuffer;
  for(int i=0; i<cube_num_verts; i++)
    {
      *vb = (*vb)*xwidth + xmin; vb++;
      *vb = (*vb)*ywidth + ymin; vb++;
      *vb = (*vb)*zwidth + zmin; vb++;
    }

  // Insert the trimesh
  createTriShape(cube_num_tris*3, cube_tris,
                 cube_num_verts, vbuffer,
                 trans, matrix);
}
*/

// Create a triangle shape and insert it into the current index/vertex
// array. If no array is active, create one.
extern "C" void bullet_createTriShape(int32_t numFaces,
                                      void *triArray,
                                      int32_t numVerts,
                                      void *vertArray,
                                      float *trans,
                                      float *matrix)
{
  createTriShape(numFaces, copyBuffer(triArray, 2, numFaces),
                 numVerts, copyBuffer(vertArray, 12, numVerts),
                 trans, matrix);
}

// Get the shape built up so far, if any. This clears g_currentMesh,
// so the next call to createTriShape will start a new shape.
extern "C" btCollisionShape *bullet_getFinalShape()
{
  btCollisionShape *shape;

  // Create a shape from all the inserted completed meshes
  shape = NULL;
  if(g_currentMesh != NULL)
    shape = new btBvhTriangleMeshShape(g_currentMesh, false);

  // Is there a compound shape as well? (usually contains bounding
  // boxes)
  if(g_compoundShape != NULL)
    {
      // If both shape types are present, insert the mesh shape into
      // the compound.
      if(shape != NULL)
        {
          btTransform tr;
          tr.setIdentity();
          g_compoundShape->addChildShape(tr, shape);
        }

      // The compound is the final shape
      shape = g_compoundShape;
    }

  // Clear these for the next NIF
  g_currentMesh = NULL;
  g_compoundShape = NULL;
  return shape;
}

// Insert a static mesh
extern "C" void bullet_insertStatic(btCollisionShape *shape,
                                    float *pos,
                                    float *quat,
                                    float scale)
{
  // Are we scaled?
  if(scale != 1.0)
    return;
    // Wrap the shape in a class that scales it. TODO: Try this on
    // ALL shapes, just to test the slowdown.
    //shape = new ScaleShape(shape, scale);

  btTransform trafo;
  trafo.setIdentity();
  trafo.setOrigin(btVector3(pos[0], pos[1], pos[2]));

  // Ogre uses WXYZ quaternions, Bullet uses XYZW.
  trafo.setRotation(btQuaternion(quat[1], quat[2], quat[3], quat[0]));

  // Create and insert the collision object
  btCollisionObject *obj = new btCollisionObject();
  obj->setCollisionShape(shape);
  obj->setWorldTransform(trafo);
  g_dynamicsWorld->addCollisionObject(obj, MASK_STATIC, MASK_PLAYER);
}

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
