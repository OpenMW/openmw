/*
Bullet Continuous Collision Detection and Physics Library
Copyright (c) 2003-2008 Erwin Coumans  http://bulletphysics.com

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it freely,
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#include "LinearMath/btIDebugDraw.h"
#include "BulletCollision/CollisionDispatch/btGhostObject.h"
#include "BulletCollision/CollisionShapes/btMultiSphereShape.h"
#include "BulletCollision/BroadphaseCollision/btOverlappingPairCache.h"
#include "BulletCollision/BroadphaseCollision/btCollisionAlgorithm.h"
#include "BulletCollision/CollisionDispatch/btCollisionWorld.h"
#include "LinearMath/btDefaultMotionState.h"
#include "btKinematicCharacterController.h"

///@todo Interact with dynamic objects,
///Ride kinematicly animated platforms properly
///Support ducking
class btKinematicClosestNotMeRayResultCallback : public btCollisionWorld::ClosestRayResultCallback
{
public:
    btKinematicClosestNotMeRayResultCallback (btCollisionObject* me) : btCollisionWorld::ClosestRayResultCallback(btVector3(0.0, 0.0, 0.0), btVector3(0.0, 0.0, 0.0))
    {
        m_me[0] = me;
        count = 1;
    }

    btKinematicClosestNotMeRayResultCallback (btCollisionObject* me[], int count_) : btCollisionWorld::ClosestRayResultCallback(btVector3(0.0, 0.0, 0.0), btVector3(0.0, 0.0, 0.0))
    {
        count = count_;

        for(int i = 0; i < count; i++)
            m_me[i] = me[i];
    }

    virtual btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult,bool normalInWorldSpace)
    {
        for(int i = 0; i < count; i++)
            if (rayResult.m_collisionObject == m_me[i])
                return 1.0;

        return ClosestRayResultCallback::addSingleResult (rayResult, normalInWorldSpace);
    }
protected:
    btCollisionObject* m_me[10];
    int count;
};

class btKinematicClosestNotMeConvexResultCallback : public btCollisionWorld::ClosestConvexResultCallback
{
public:
    btKinematicClosestNotMeConvexResultCallback( btCollisionObject* me, const btVector3& up, btScalar minSlopeDot )
        : btCollisionWorld::ClosestConvexResultCallback( btVector3( 0.0, 0.0, 0.0 ), btVector3( 0.0, 0.0, 0.0 ) ),
        m_me( me ), m_up( up ), m_minSlopeDot( minSlopeDot )
    {
    }

    virtual btScalar addSingleResult(btCollisionWorld::LocalConvexResult& convexResult,bool normalInWorldSpace)
    {
        if( convexResult.m_hitCollisionObject == m_me )
            return btScalar( 1 );

        btVector3 hitNormalWorld;
        if( normalInWorldSpace )
        {
            hitNormalWorld = convexResult.m_hitNormalLocal;
        }
        else
        {
            ///need to transform normal into worldspace
            hitNormalWorld = m_hitCollisionObject->getWorldTransform().getBasis()*convexResult.m_hitNormalLocal;
        }

        // NOTE : m_hitNormalLocal is not always vertical on the ground with a capsule or a box...

        btScalar dotUp = m_up.dot(hitNormalWorld);
        if( dotUp < m_minSlopeDot )
            return btScalar( 1 );

        return ClosestConvexResultCallback::addSingleResult (convexResult, normalInWorldSpace);
    }

protected:
    btCollisionObject* m_me;
    const btVector3 m_up;
    btScalar m_minSlopeDot;
};



btKinematicCharacterController::btKinematicCharacterController( btPairCachingGhostObject* externalGhostObject_,
    btPairCachingGhostObject* internalGhostObject_,
    btScalar stepHeight,
    btScalar constantScale,
    btScalar gravity,
    btScalar fallVelocity,
    btScalar jumpVelocity,
    btScalar recoveringFactor )
{
    m_upAxis = btKinematicCharacterController::Y_AXIS;

    m_walkDirection.setValue( btScalar( 0 ), btScalar( 0 ), btScalar( 0 ) );

    m_useGhostObjectSweepTest = true;

    externalGhostObject = externalGhostObject_;
    internalGhostObject = internalGhostObject_;

    m_recoveringFactor = recoveringFactor;

    m_stepHeight = stepHeight;

    m_useWalkDirection = true;  // use walk direction by default, legacy behavior
    m_velocityTimeInterval = btScalar( 0 );
    m_verticalVelocity = btScalar( 0 );
    m_verticalOffset = btScalar( 0 );

    m_gravity = constantScale * gravity;
    m_fallSpeed = constantScale * fallVelocity; // Terminal velocity of a sky diver in m/s.

    m_jumpSpeed = constantScale * jumpVelocity; // ?
    m_wasJumping = false;

    setMaxSlope( btRadians( 45.0 ) );

    mCollision = true;
}


btKinematicCharacterController::~btKinematicCharacterController ()
{
}

void btKinematicCharacterController::setVerticalVelocity(float z)
{
    m_verticalVelocity = z;
}

bool btKinematicCharacterController::recoverFromPenetration( btCollisionWorld* collisionWorld )
{
    bool penetration = false;

    if(!mCollision) return penetration;

    collisionWorld->getDispatcher()->dispatchAllCollisionPairs( internalGhostObject->getOverlappingPairCache(),
        collisionWorld->getDispatchInfo(),
        collisionWorld->getDispatcher() );

    btVector3 currentPosition = internalGhostObject->getWorldTransform().getOrigin();

    btScalar maxPen = btScalar( 0 );

    for( int i = 0; i < internalGhostObject->getOverlappingPairCache()->getNumOverlappingPairs(); i++ )
    {
        m_manifoldArray.resize(0);

        btBroadphasePair* collisionPair = &internalGhostObject->getOverlappingPairCache()->getOverlappingPairArray()[i];

        if( collisionPair->m_algorithm )
            collisionPair->m_algorithm->getAllContactManifolds( m_manifoldArray );


        for( int j = 0; j < m_manifoldArray.size(); j++ )
        {
            btPersistentManifold* manifold = m_manifoldArray[j];

            btScalar directionSign = manifold->getBody0() == internalGhostObject ? btScalar( -1.0 ) : btScalar( 1.0 );

            for( int p = 0; p < manifold->getNumContacts(); p++ )
            {
                const btManifoldPoint&pt = manifold->getContactPoint( p );
                if( (manifold->getBody1() == externalGhostObject && manifold->getBody0() == internalGhostObject)
                    ||(manifold->getBody0() == externalGhostObject && manifold->getBody1() == internalGhostObject) )
                {
                }
                else
                {
                    btScalar dist = pt.getDistance();

                    if( dist < 0.0 )
                    {
                        if( dist < maxPen )
                            maxPen = dist;

                        // NOTE : btScalar affects the stairs but the parkinson...
                        // 0.0 , the capsule can break the walls...
                        currentPosition += pt.m_normalWorldOnB * directionSign * dist * m_recoveringFactor;

                        penetration = true;
                    }
                }
            }

            // ???
            //manifold->clearManifold();
        }
    }

    btTransform transform = internalGhostObject->getWorldTransform();

    transform.setOrigin( currentPosition );

    internalGhostObject->setWorldTransform( transform );
    externalGhostObject->setWorldTransform( transform );

    return penetration;
}


btVector3 btKinematicCharacterController::stepUp( btCollisionWorld* world, const btVector3& currentPosition, btScalar& currentStepOffset )
{
    btVector3 targetPosition = currentPosition + getUpAxisDirections()[ m_upAxis ] * ( m_stepHeight + ( m_verticalOffset > btScalar( 0.0 ) ? m_verticalOffset : 0.0 ) );

    //if the no collisions mode is on, no need to go any further
    if(!mCollision)
    {
        currentStepOffset = m_stepHeight;
        return targetPosition;
    }

    // Retrieve the collision shape
    //
    btCollisionShape* collisionShape = externalGhostObject->getCollisionShape();
    btAssert( collisionShape->isConvex() );

    btConvexShape* convexShape = ( btConvexShape* )collisionShape;

    // FIXME: Handle penetration properly
    //
    btTransform start;
    start.setIdentity();
    start.setOrigin( currentPosition + getUpAxisDirections()[ m_upAxis ] * ( convexShape->getMargin() ) );

    btTransform end;
    end.setIdentity();
    end.setOrigin( targetPosition );

    btKinematicClosestNotMeConvexResultCallback callback( externalGhostObject, -getUpAxisDirections()[ m_upAxis ], m_maxSlopeCosine );
    callback.m_collisionFilterGroup = externalGhostObject->getBroadphaseHandle()->m_collisionFilterGroup;
    callback.m_collisionFilterMask = externalGhostObject->getBroadphaseHandle()->m_collisionFilterMask;

    // Sweep test
    //
    if( m_useGhostObjectSweepTest )
        externalGhostObject->convexSweepTest( convexShape, start, end, callback, world->getDispatchInfo().m_allowedCcdPenetration );

    else
        world->convexSweepTest( convexShape, start, end, callback );

    if( callback.hasHit() )
    {
        // Only modify the position if the hit was a slope and not a wall or ceiling.
        //
        if( callback.m_hitNormalWorld.dot(getUpAxisDirections()[m_upAxis]) > btScalar( 0.0 ) )
        {
            // We moved up only a fraction of the step height
            //
            currentStepOffset = m_stepHeight * callback.m_closestHitFraction;

            return currentPosition.lerp( targetPosition, callback.m_closestHitFraction );
        }

        m_verticalVelocity = btScalar( 0.0 );
        m_verticalOffset = btScalar( 0.0 );

        return currentPosition;
    }
    else
    {
        currentStepOffset = m_stepHeight;
        return targetPosition;
    }
}


///Reflect the vector d around the vector r
inline btVector3 reflect( const btVector3& d, const btVector3& r )
{
    return d - ( btScalar( 2.0 ) * d.dot( r ) ) * r;
}


///Project a vector u on another vector v
inline btVector3 project( const btVector3& u, const btVector3& v )
{
    return v * u.dot( v );
}


///Helper for computing the character sliding
inline btVector3 slide( const btVector3& direction, const btVector3& planeNormal )
{
    return direction - project( direction, planeNormal );
}



btVector3 slideOnCollision( const btVector3& fromPosition, const btVector3& toPosition, const btVector3& hitNormal )
{
    btVector3 moveDirection = toPosition - fromPosition;
    btScalar moveLength = moveDirection.length();

    if( moveLength <= btScalar( SIMD_EPSILON ) )
        return toPosition;

    moveDirection.normalize();

    btVector3 reflectDir = reflect( moveDirection, hitNormal );
    reflectDir.normalize();

    return fromPosition + slide( reflectDir, hitNormal ) * moveLength;
}


btVector3 btKinematicCharacterController::stepForwardAndStrafe( btCollisionWorld* collisionWorld, const btVector3& currentPosition, const btVector3& walkMove )
{
    // We go to !
    //
    btVector3 targetPosition = currentPosition + walkMove;

    //if the no collisions mode is on, no need to go any further
    if(!mCollision) return targetPosition;

    // Retrieve the collision shape
    //
    btCollisionShape* collisionShape = externalGhostObject->getCollisionShape();
    btAssert( collisionShape->isConvex() );

    btConvexShape* convexShape = ( btConvexShape* )collisionShape;

    btTransform start;
    start.setIdentity();

    btTransform end;
    end.setIdentity();

    btScalar fraction = btScalar( 1.0 );

    // This optimization scheme suffers in the corners.
    // It basically jumps from a wall to another, then fails to find a new
    // position (after 4 iterations here) and finally don't move at all.
    //
    // The stepping algorithm adds some problems with stairs. It seems
    // the treads create some fake corner using capsules for collisions.
    //
    for( int i = 0; i < 4 && fraction > btScalar( 0.01 ); i++ )
    {
        start.setOrigin( currentPosition );
        end.setOrigin( targetPosition );

        btVector3 sweepDirNegative = currentPosition - targetPosition;

        btKinematicClosestNotMeConvexResultCallback callback( externalGhostObject, sweepDirNegative, btScalar( 0.0 ) );
        callback.m_collisionFilterGroup = externalGhostObject->getBroadphaseHandle()->m_collisionFilterGroup;
        callback.m_collisionFilterMask = externalGhostObject->getBroadphaseHandle()->m_collisionFilterMask;

        if( m_useGhostObjectSweepTest )
            externalGhostObject->convexSweepTest( convexShape, start, end, callback, collisionWorld->getDispatchInfo().m_allowedCcdPenetration );

        else
            collisionWorld->convexSweepTest( convexShape, start, end, callback, collisionWorld->getDispatchInfo().m_allowedCcdPenetration );

        if( callback.hasHit() )
        {
            // Try another target position
            //
            targetPosition = slideOnCollision( currentPosition, targetPosition, callback.m_hitNormalWorld );
            fraction = callback.m_closestHitFraction;
        }
        else

            // Move to the valid target position
            //
            return targetPosition;
    }

    // Don't move if you can't find a valid target position...
    // It prevents some flickering.
    //
    return currentPosition;
}


///Handle the gravity
btScalar btKinematicCharacterController::addFallOffset( bool wasOnGround, btScalar currentStepOffset, btScalar dt )
{
    btScalar downVelocity = ( m_verticalVelocity < 0.0 ? -m_verticalVelocity : btScalar( 0.0 ) ) * dt;

    if( downVelocity > btScalar( 0.0 ) && downVelocity < m_stepHeight && ( wasOnGround || !m_wasJumping ) )
        downVelocity = m_stepHeight;

    return currentStepOffset + downVelocity;
}


btVector3 btKinematicCharacterController::stepDown( btCollisionWorld* collisionWorld, const btVector3& currentPosition, btScalar currentStepOffset )
{
    btVector3 stepDrop = getUpAxisDirections()[ m_upAxis ] * currentStepOffset;

    // Be sure we are falling from the last m_currentPosition
    // It prevents some flickering
    //
    btVector3 targetPosition = currentPosition - stepDrop;

    //if the no collisions mode is on, no need to go any further
    if(!mCollision) return targetPosition;

    btTransform start;
    start.setIdentity();
    start.setOrigin( currentPosition );

    btTransform end;
    end.setIdentity();
    end.setOrigin( targetPosition );

    btKinematicClosestNotMeConvexResultCallback callback( internalGhostObject, getUpAxisDirections()[ m_upAxis ], m_maxSlopeCosine );
    callback.m_collisionFilterGroup = internalGhostObject->getBroadphaseHandle()->m_collisionFilterGroup;
    callback.m_collisionFilterMask = internalGhostObject->getBroadphaseHandle()->m_collisionFilterMask;

    // Retrieve the collision shape
    //
    btCollisionShape* collisionShape = internalGhostObject->getCollisionShape();
    btAssert( collisionShape->isConvex() );
    btConvexShape* convexShape = ( btConvexShape* )collisionShape;

    if( m_useGhostObjectSweepTest )
        externalGhostObject->convexSweepTest( convexShape, start, end, callback, collisionWorld->getDispatchInfo().m_allowedCcdPenetration );

    else
        collisionWorld->convexSweepTest( convexShape, start, end, callback, collisionWorld->getDispatchInfo().m_allowedCcdPenetration );

    if( callback.hasHit() )
    {
        m_verticalVelocity = btScalar( 0.0 );
        m_verticalOffset = btScalar( 0.0 );
        m_wasJumping = false;

        // We dropped a fraction of the height -> hit floor
        //
        return currentPosition.lerp( targetPosition, callback.m_closestHitFraction );
    }
    else

        // We dropped the full height
        //
        return targetPosition;
}



void btKinematicCharacterController::setWalkDirection( const btVector3& walkDirection )
{
    m_useWalkDirection = true;
    m_walkDirection = walkDirection;
}


void btKinematicCharacterController::setVelocityForTimeInterval( const btVector3& velocity, btScalar timeInterval )
{
    m_useWalkDirection = false;
    m_walkDirection = velocity;
    m_velocityTimeInterval = timeInterval;
}


void btKinematicCharacterController::reset()
{
}


void btKinematicCharacterController::warp( const btVector3& origin )
{
    btTransform transform;
    transform.setIdentity();
    transform.setOrigin( -origin );

    externalGhostObject->setWorldTransform( transform );
    internalGhostObject->setWorldTransform( transform );
}


void btKinematicCharacterController::preStep( btCollisionWorld* collisionWorld )
{
    BT_PROFILE( "preStep" );

    for( int i = 0; i < 4 && recoverFromPenetration ( collisionWorld ); i++ );
}


void btKinematicCharacterController::playerStep( btCollisionWorld* collisionWorld, btScalar dt )
{
    BT_PROFILE( "playerStep" );

    if( !m_useWalkDirection && m_velocityTimeInterval <= btScalar( 0.0 ) )
        return;

    bool wasOnGround = onGround();

    // Handle the gravity
    //
    m_verticalVelocity -= m_gravity * dt;

    if( m_verticalVelocity > 0.0 && m_verticalVelocity > m_jumpSpeed )
        m_verticalVelocity = m_jumpSpeed;

    if( m_verticalVelocity < 0.0 && btFabs( m_verticalVelocity ) > btFabs( m_fallSpeed ) )
        m_verticalVelocity = -btFabs( m_fallSpeed );

    m_verticalOffset = m_verticalVelocity * dt;

    // This forced stepping up can cause problems when the character
    // walks (jump in fact...) under too low ceilings.
    //
    btVector3 currentPosition = externalGhostObject->getWorldTransform().getOrigin();
    btScalar currentStepOffset;

    currentPosition = stepUp( collisionWorld, currentPosition, currentStepOffset );

    // Move in the air and slide against the walls ignoring the stair steps.
    //
    if( m_useWalkDirection )
        currentPosition = stepForwardAndStrafe( collisionWorld, currentPosition, m_walkDirection );

    else
    {
        btScalar dtMoving = ( dt < m_velocityTimeInterval ) ? dt : m_velocityTimeInterval;
        m_velocityTimeInterval -= dt;

        // How far will we move while we are moving ?
        //
        btVector3 moveDirection = m_walkDirection * dtMoving;

        currentPosition = stepForwardAndStrafe( collisionWorld, currentPosition, moveDirection );
    }

    // Finally find the ground.
    //
    currentStepOffset = addFallOffset( wasOnGround, currentStepOffset, dt );

    currentPosition = stepDown( collisionWorld, currentPosition, currentStepOffset );

    // Apply the new position to the collision objects.
    //
    btTransform tranform;
    tranform = externalGhostObject->getWorldTransform();
    tranform.setOrigin( currentPosition );

    externalGhostObject->setWorldTransform( tranform );
    internalGhostObject->setWorldTransform( tranform );
}


void btKinematicCharacterController::setFallSpeed( btScalar fallSpeed )
{
    m_fallSpeed = fallSpeed;
}


void btKinematicCharacterController::setJumpSpeed( btScalar jumpSpeed )
{
    m_jumpSpeed = jumpSpeed;
}


void btKinematicCharacterController::setMaxJumpHeight( btScalar maxJumpHeight )
{
    m_maxJumpHeight = maxJumpHeight;
}


bool btKinematicCharacterController::canJump() const
{
    return onGround();
}


void btKinematicCharacterController::jump()
{
    if( !canJump() )
        return;

    m_verticalVelocity = m_jumpSpeed;
    m_wasJumping = true;
}


void btKinematicCharacterController::setGravity( btScalar gravity )
{
    m_gravity = gravity;
}


btScalar btKinematicCharacterController::getGravity() const
{
    return m_gravity;
}


void btKinematicCharacterController::setMaxSlope( btScalar slopeRadians )
{
    m_maxSlopeRadians = slopeRadians;
    m_maxSlopeCosine = btCos( slopeRadians );
}


btScalar btKinematicCharacterController::getMaxSlope() const
{
    return m_maxSlopeRadians;
}


bool btKinematicCharacterController::onGround() const
{
    return btFabs( m_verticalVelocity ) < btScalar( SIMD_EPSILON ) &&
        btFabs( m_verticalOffset ) < btScalar( SIMD_EPSILON );
}


btVector3* btKinematicCharacterController::getUpAxisDirections()
{
    static btVector3 sUpAxisDirection[] =
    {
        btVector3( btScalar( 0.0 ), btScalar( 0.0 ), btScalar( 0.0 ) ),
        btVector3( btScalar( 0.0 ), btScalar( 1.0 ), btScalar( 0.0 ) ),
        btVector3( btScalar( 0.0 ), btScalar( 0.0 ), btScalar( 1.0 ) )
    };

    return sUpAxisDirection;
}


void btKinematicCharacterController::debugDraw( btIDebugDraw* debugDrawer )
{
}
