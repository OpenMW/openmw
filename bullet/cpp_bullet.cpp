#include "btBulletCollisionCommon.h"

#include <iostream>

using namespace std;

btDefaultCollisionConfiguration* m_collisionConfiguration;
btCollisionDispatcher *m_dispatcher;
btBroadphaseInterface *m_broadphase;
//btSequentialImpulseConstraintSolver* m_solver;
//btDynamicsWorld *m_dynamicsWorld;
btCollisionWorld *m_collisionWorld;

//btCollisionObject* m_playerObject;
btConvexShape *playerShape;

extern "C" int32_t cpp_initBullet()
{
  // Set up basic objects
  m_collisionConfiguration = new btDefaultCollisionConfiguration();
  m_dispatcher = new btCollisionDispatcher(m_collisionConfiguration);
  m_broadphase = new btDbvtBroadphase();
  m_collisionWorld =
    new btCollisionWorld(m_dispatcher, m_broadphase,
                         m_collisionConfiguration);

  /*m_solver = new btSequentialImpulseConstraintSolver;
  m_dynamicsWorld =
    new btDiscreteDynamicsWorld(m_dispatcher, m_broadphase,
                                m_solver, m_collisionConfiguration);
  m_dynamicsWorld->setGravity(btVector3(0,-10,0));
  */

  // Create player collision object
  //playerShape = new btCylinderShape(btVector3(50, 50, 50));
  playerShape = new btSphereShape(50);

  /*
  m_playerObject = new btCollisionObject ();
  m_playerObject->setCollisionShape (m_shape);
  m_playerObject->setCollisionFlags (btCollisionObject::CF_NO_CONTACT_RESPONSE);
  */

  /*
  // Dynamic shapes:

  // Re-using the same collision is better for memory usage and
  // performance.

  btCollisionShape* colShape = new btBoxShape(btVector3(1,1,1));
  //btCollisionShape* colShape = new btSphereShape(btScalar(1.));
  //m_collisionShapes.push_back(colShape);

  // Create Dynamic Objects
  btTransform startTransform;
  startTransform.setIdentity();

  mass = 1.f;

  colShape->calculateLocalInertia(mass,localInertia);

///create 125 (5x5x5) dynamic objects
#define ARRAY_SIZE_X 5
#define ARRAY_SIZE_Y 5
#define ARRAY_SIZE_Z 5

#define START_POS_X -5
#define START_POS_Y -5
#define START_POS_Z -3

  float start_x = START_POS_X - ARRAY_SIZE_X/2;
  float start_y = START_POS_Y;
  float start_z = START_POS_Z - ARRAY_SIZE_Z/2;

  for (int k=0;k<ARRAY_SIZE_Y;k++)
    for (int i=0;i<ARRAY_SIZE_X;i++)
      for(int j = 0;j<ARRAY_SIZE_Z;j++)
        {
          startTransform.setOrigin(btVector3(2.0*i + start_x,
                                             10+2.0*k + start_y,
                                             2.0*j + start_z));

          btDefaultMotionState* myMotionState =
            new btDefaultMotionState(startTransform);

          btRigidBody::btRigidBodyConstructionInfo
            rbInfo(mass,myMotionState,colShape,localInertia);

          btRigidBody* body = new btRigidBody(rbInfo);
          body->setActivationState(ISLAND_SLEEPING);

          m_dynamicsWorld->addRigidBody(body);
          body->setActivationState(ISLAND_SLEEPING);
        }
  */
  return 0;
}

extern "C" int32_t cpp_movePlayerCollision(float x, float y, float z,
                                        float dx, float dy, float dz)
{
  btTransform start, end;
  start.setIdentity();
  end.setIdentity();

  // The sweep test moves the shape from the old position to the
  // new. The 0.1 offset in one of the coordinates is to make sure a
  // sweep is performed even when the player does not move.
  start.setOrigin(btVector3(x, y, z));
  end.setOrigin(btVector3(x+dx,y+dy,z+dz));

  btCollisionWorld::ClosestConvexResultCallback cb(btVector3(0,0,0),
                                            btVector3(0,0,0));

  m_collisionWorld->convexSweepTest(playerShape, start, end, cb);

  if(cb.hasHit()) return 1;
  else return 0;
}

extern "C" void cpp_insertBox(float x, float y, float z)
{
  btCollisionShape* groundShape =
    new btSphereShape(50);
    //new btBoxShape(btVector3(100,100,100));

  btTransform groundTransform;
  groundTransform.setIdentity();
  groundTransform.setOrigin(btVector3(x,y,z));

  btCollisionObject *obj = new btCollisionObject;
  obj->setCollisionShape(groundShape);
  obj->setWorldTransform(groundTransform);

  m_collisionWorld->addCollisionObject(obj);

  /*
  m_collisionWorld->addCollisionObject(obj,
      btBroadphaseProxy::DebrisFilter,  // ??
      btBroadphaseProxy::StaticFilter); // Only static objects

  /*
  btDefaultMotionState* myMotionState =
    new btDefaultMotionState(groundTransform);

  btRigidBody::btRigidBodyConstructionInfo
    rbInfo(0, myMotionState, groundShape, btVector3(0,0,0));

  btRigidBody* body = new btRigidBody(rbInfo);

  // Add the body to the dynamics world
  m_dynamicsWorld->addRigidBody(body);
  */
}

/*
extern "C" void cpp_timeStep(float delta)
{
  // TODO: Find what unit Bullet uses here
  m_dynamicsWorld->stepSimulation(delta / 1000.f);
}
*/

// Cleanup in the reverse order of creation/initialization
extern "C" void cpp_cleanupBullet()
{
  // Remove the rigidbodies from the dynamics world and delete them
  for (int i=m_collisionWorld->getNumCollisionObjects()-1; i>=0 ;i--)
    {
      btCollisionObject* obj = m_collisionWorld->getCollisionObjectArray()[i];
      /*
      btRigidBody* body = btRigidBody::upcast(obj);

      if (body && body->getMotionState())
        delete body->getMotionState();
      */

      m_collisionWorld->removeCollisionObject( obj );
      delete obj;
    }

  // Delete collision shapes
  /*
  for (int j=0;j<m_collisionShapes.size();j++)
    {
      btCollisionShape* shape = m_collisionShapes[j];
      delete shape;
    }
  */

  delete m_collisionWorld;
  //delete m_solver;
  delete m_broadphase;
  delete m_dispatcher;
  delete m_collisionConfiguration;
}
