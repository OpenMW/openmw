/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (cpp_player.cpp) is part of the OpenMW package.

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

// This file handles player-specific physics and collision detection

// TODO: Later we might handle various physics modes, eg. dynamic
// (full physics), player_walk, player_fall, player_swim, player_float,
// player_levitate, player_ghost. These would be applicable to any
// object (through Monster script), allowing player physics to be used
// on NPCs and creatures.

// Variables used internally in this file. Once we make per-object
// player collision, these will be member variables.
bool g_touchingContact;
btVector3 g_touchingNormal;
btScalar g_currentStepOffset;
float g_stepHeight = 20;

// Returns the reflection direction of a ray going 'direction' hitting
// a surface with normal 'normal'
btVector3 reflect (const btVector3& direction, const btVector3& normal)
{ return direction - (btScalar(2.0) * direction.dot(normal)) * normal; }

// Returns the portion of 'direction' that is perpendicular to
// 'normal'
btVector3 perpComponent (const btVector3& direction, const btVector3& normal)
{ return direction - normal * direction.dot(normal); }

btManifoldArray manifoldArray;

// Callback used for collision detection sweep tests. It prevents self
// collision and is used in calls to convexSweepTest().
class ClosestNotMeConvexResultCallback : public btCollisionWorld::ClosestConvexResultCallback
{
public:
  ClosestNotMeConvexResultCallback()
    : btCollisionWorld::ClosestConvexResultCallback
      (btVector3(0.0, 0.0, 0.0), btVector3(0.0, 0.0, 0.0))
  {
    m_collisionFilterGroup = g_playerObject->
      getBroadphaseHandle()->m_collisionFilterGroup;

    m_collisionFilterMask = g_playerObject->
      getBroadphaseHandle()->m_collisionFilterMask;
  }

  btScalar addSingleResult(btCollisionWorld::LocalConvexResult&
                           convexResult, bool normalInWorldSpace)
  {
    if (convexResult.m_hitCollisionObject == g_playerObject) return 1.0;

    return ClosestConvexResultCallback::addSingleResult
      (convexResult, normalInWorldSpace);
  }
};

// Used to step up small steps and slopes.
void stepUp()
{
  // phase 1: up
  btVector3 targetPosition = g_playerPosition +
    btVector3(0.0, 0.0, g_stepHeight);
  btTransform start, end;

  start.setIdentity ();
  end.setIdentity ();

  // FIXME: Handle penetration properly
  start.setOrigin (g_playerPosition + btVector3(0.0, 0.1, 0.0));
  end.setOrigin (targetPosition);

  ClosestNotMeConvexResultCallback callback;
  g_dynamicsWorld->convexSweepTest (g_playerShape, start, end, callback);

  if (callback.hasHit())
    {
      // we moved up only a fraction of the step height
      g_currentStepOffset = g_stepHeight * callback.m_closestHitFraction;
      g_playerPosition.setInterpolate3(g_playerPosition, targetPosition,
                                       callback.m_closestHitFraction);
    }
  else
    {
      g_currentStepOffset = g_stepHeight;
      g_playerPosition = targetPosition;
    }
}

void updateTargetPositionBasedOnCollision (const btVector3& hitNormal,
                                           btVector3 &targetPosition)
{
  btVector3 movementDirection = targetPosition - g_playerPosition;
  btScalar movementLength = movementDirection.length();

  if (movementLength <= SIMD_EPSILON)
    return;

  // Is this needed?
  movementDirection.normalize();

  btVector3 reflectDir = reflect(movementDirection, hitNormal);
  reflectDir.normalize();

  btVector3 perpendicularDir = perpComponent (reflectDir, hitNormal);

  targetPosition = g_playerPosition;
  targetPosition += perpendicularDir * movementLength;
}

// This covers all normal forward movement and collision, including
// walking sideways when hitting a wall at an angle. It does NOT
// handle walking up slopes and steps, or falling/gravity.
void stepForward(btVector3& walkMove)
{
  btVector3 originalDir = walkMove.normalized();

  // If no walking direction is given, we still run the function. This
  // allows moving forces to push the player around even if she is
  // standing still.
  if (walkMove.length() < SIMD_EPSILON)
    originalDir.setValue(0.f,0.f,0.f);

  btTransform start, end;
  btVector3 targetPosition = g_playerPosition + walkMove;
  start.setIdentity ();
  end.setIdentity ();
	
  btScalar fraction = 1.0;
  btScalar distance2 = (g_playerPosition-targetPosition).length2();

  if (g_touchingContact)
    if (originalDir.dot(g_touchingNormal) > btScalar(0.0))
      updateTargetPositionBasedOnCollision (g_touchingNormal, targetPosition);

  int maxIter = 10;

  while (fraction > btScalar(0.01) && maxIter-- > 0)
    {
      start.setOrigin (g_playerPosition);
      end.setOrigin (targetPosition);

      ClosestNotMeConvexResultCallback callback;
      g_dynamicsWorld->convexSweepTest (g_playerShape, start, end, callback);
		
      fraction -= callback.m_closestHitFraction;

      if (callback.hasHit())
        {	
          // We moved only a fraction
          btScalar hitDistance = (callback.m_hitPointWorld - g_playerPosition).length();
          // If the distance is further than the collision margin,
          // move
          if (hitDistance > 0.05)
            g_playerPosition.setInterpolate3(g_playerPosition, targetPosition,
                                             callback.m_closestHitFraction);

          updateTargetPositionBasedOnCollision(callback.m_hitNormalWorld,
                                               targetPosition);
          btVector3 currentDir = targetPosition - g_playerPosition;
          distance2 = currentDir.length2();

          if (distance2 <= SIMD_EPSILON)
            break;

          currentDir.normalize();

          if (currentDir.dot(originalDir) <= btScalar(0.0))
            break;
        }
      else
        // we moved the whole way
        g_playerPosition = targetPosition;
    }
}

void stepDown (btScalar dt)
{
  btTransform start, end;

  // phase 3: down
  btVector3 step_drop = btVector3(0,0,g_currentStepOffset);
  btVector3 gravity_drop = btVector3(0,0,g_stepHeight);

  btVector3 targetPosition = g_playerPosition - step_drop - gravity_drop;

  start.setIdentity ();
  end.setIdentity ();

  start.setOrigin (g_playerPosition);
  end.setOrigin (targetPosition);

  ClosestNotMeConvexResultCallback callback;
  g_dynamicsWorld->convexSweepTest(g_playerShape, start, end, callback);

  if (callback.hasHit())
    // we dropped a fraction of the height -> hit floor
    g_playerPosition.setInterpolate3(g_playerPosition, targetPosition,
                                     callback.m_closestHitFraction);
  else
    // we dropped the full height
    g_playerPosition = targetPosition;
}

// Check if the player currently collides with anything, and adjust
// its position accordingly. Returns true if collisions were found.
bool recoverFromPenetration()
{
  bool penetration = false;

  // Update the collision pair cache
  g_dispatcher->dispatchAllCollisionPairs(g_pairCache,
                                          g_dynamicsWorld->getDispatchInfo(),
                                          g_dispatcher);

  g_playerPosition = g_playerObject->getWorldTransform().getOrigin();
	
  btScalar maxPen = 0.0;
  for (int i = 0; i < g_pairCache->getNumOverlappingPairs(); i++)
    {
      manifoldArray.resize(0);

      btBroadphasePair* collisionPair = &g_pairCache->getOverlappingPairArray()[i];
      // Get the contact points
      if (collisionPair->m_algorithm)
        collisionPair->m_algorithm->getAllContactManifolds(manifoldArray);

      // And handle them
      for (int j=0;j<manifoldArray.size();j++)
        {
          btPersistentManifold* manifold = manifoldArray[j];
          btScalar directionSign = manifold->getBody0() ==
            g_playerObject ? btScalar(-1.0) : btScalar(1.0);

          for (int p=0;p<manifold->getNumContacts();p++)
            {
              const btManifoldPoint &pt = manifold->getContactPoint(p);

              if (pt.getDistance() < 0.0)
                {
                  // Pick out the maximum penetration normal and store
                  // it
                  if (pt.getDistance() < maxPen)
                    {
                      maxPen = pt.getDistance();
                      g_touchingNormal = pt.m_normalWorldOnB * directionSign;//??

                    }
                  g_playerPosition += pt.m_normalWorldOnB * directionSign *
                    pt.getDistance() * btScalar(0.2);

                  penetration = true;
                }
            }
        }
    }

  btTransform newTrans = g_playerObject->getWorldTransform();
  newTrans.setOrigin(g_playerPosition);
  g_playerObject->setWorldTransform(newTrans);

  return penetration;
}

// Callback called at the end of each simulation cycle. This is the
// main function is responsible for player movement.
void playerStepCallback(btDynamicsWorld* dynamicsWorld, btScalar timeStep)
{
  // Before moving, recover from current penetrations

  int numPenetrationLoops = 0;
  g_touchingContact = false;
  while (recoverFromPenetration())
    {
      numPenetrationLoops++;
      g_touchingContact = true;

      // Make sure we don't stay here indefinitely
      if (numPenetrationLoops > 4)
        break;
    }

  // Get the player position
  btTransform xform;
  xform = g_playerObject->getWorldTransform ();
  g_playerPosition = xform.getOrigin();

  // Next, do the walk.

  // TODO: Walking direction should be set from D code, and the final
  // position should be retrieved back from C++. The walking direction
  // always reflects the intentional action of an agent (ie. the
  // player or the AI), but the end result comes from collision and
  // physics.

  btVector3 walkStep = g_walkDirection * timeStep;

  stepUp();
  stepForward(walkStep);
  stepDown(timeStep);

  // Move the player (but keep rotation)
  xform = g_playerObject->getWorldTransform ();
  xform.setOrigin (g_playerPosition);
  g_playerObject->setWorldTransform (xform);
}
