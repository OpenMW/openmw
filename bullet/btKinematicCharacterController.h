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

#ifndef KINEMATIC_CHARACTER_CONTROLLER_H
#define KINEMATIC_CHARACTER_CONTROLLER_H

#include "LinearMath/btVector3.h"
#include "LinearMath\btQuickprof.h"

#include "BulletDynamics\Character\btCharacterControllerInterface.h"

#include "BulletCollision/BroadphaseCollision/btCollisionAlgorithm.h"


class btCollisionShape;
class btRigidBody;
class btCollisionWorld;
class btCollisionDispatcher;
class btPairCachingGhostObject;

///btKinematicCharacterController is an object that supports a sliding motion in a world.
///It uses a ghost object and convex sweep test to test for upcoming collisions. This is combined with discrete collision detection to recover from penetrations.
///Interaction between btKinematicCharacterController and dynamic rigid bodies needs to be explicity implemented by the user.
class btKinematicCharacterController : public btCharacterControllerInterface
{
public:
  enum UpAxis
  {
    X_AXIS = 0,
    Y_AXIS = 1,
    Z_AXIS = 2
  };

private:
	btPairCachingGhostObject* externalGhostObject;  // use this for querying collisions for sliding and move
  btPairCachingGhostObject* internalGhostObject;  // and this for recoreving from penetrations
	
	btScalar m_verticalVelocity;
	btScalar m_verticalOffset;
	btScalar m_fallSpeed;
	btScalar m_jumpSpeed;
	btScalar m_maxJumpHeight;
	btScalar m_maxSlopeRadians; // Slope angle that is set (used for returning the exact value)
	btScalar m_maxSlopeCosine;  // Cosine equivalent of m_maxSlopeRadians (calculated once when set, for optimization)
	btScalar m_gravity;
	btScalar m_recoveringFactor;

	btScalar m_stepHeight;

	///this is the desired walk direction, set by the user
	btVector3	m_walkDirection;

	///keep track of the contact manifolds
	btManifoldArray	m_manifoldArray;

  ///Gravity attributes
	bool  m_wasJumping;

	bool	m_useGhostObjectSweepTest;
	bool	m_useWalkDirection;
	btScalar	m_velocityTimeInterval;

	UpAxis m_upAxis;

	static btVector3* getUpAxisDirections();

	bool recoverFromPenetration ( btCollisionWorld* collisionWorld );

	btVector3 stepUp( btCollisionWorld* collisionWorld, const btVector3& currentPosition, btScalar& currentStepOffset );
  btVector3 stepForwardAndStrafe( btCollisionWorld* collisionWorld, const btVector3& currentPosition, const btVector3& walkMove );
  btScalar addFallOffset( bool wasJumping, btScalar currentStepOffset, btScalar dt );
  btVector3 stepDown( btCollisionWorld* collisionWorld, const btVector3& currentPosition, btScalar currentStepOffset );

public:
  /// externalGhostObject is used for querying the collisions for sliding along the wall,
  /// and internalGhostObject is used for querying the collisions for recovering from large penetrations.
  /// These parameters can point on the same object.
  /// Using a smaller internalGhostObject can help for removing some flickering but create some
  /// stopping artefacts when sliding along stairs or small walls.
  /// Don't forget to scale gravity and fallSpeed if you scale the world.
	btKinematicCharacterController( btPairCachingGhostObject* externalGhostObject,
                                  btPairCachingGhostObject* internalGhostObject,
                                  btScalar stepHeight,
                                  btScalar constantScale = btScalar( 1.0 ),
                                  btScalar gravity = btScalar( 9.8 ),
                                  btScalar fallVelocity = btScalar( 55.0 ),
                                  btScalar jumpVelocity = btScalar( 9.8 ),
                                  btScalar recoveringFactor = btScalar( 0.2 ) );

	~btKinematicCharacterController ();
	

	///btActionInterface interface
	virtual void updateAction( btCollisionWorld* collisionWorld, btScalar deltaTime )
	{
    preStep( collisionWorld );
		playerStep( collisionWorld, deltaTime ); 
	}
	
	///btActionInterface interface
	void debugDraw( btIDebugDraw* debugDrawer );

  void setUpAxis( UpAxis axis )
	{
		m_upAxis = axis;
	}

	/// This should probably be called setPositionIncrementPerSimulatorStep.
	/// This is neither a direction nor a velocity, but the amount to
	///	increment the position each simulation iteration, regardless
	///	of dt.
	/// This call will reset any velocity set by setVelocityForTimeInterval().
	virtual void	setWalkDirection(const btVector3& walkDirection);

	/// Caller provides a velocity with which the character should move for
	///	the given time period.  After the time period, velocity is reset
	///	to zero.
	/// This call will reset any walk direction set by setWalkDirection().
	/// Negative time intervals will result in no motion.
	virtual void setVelocityForTimeInterval(const btVector3& velocity,
				btScalar timeInterval);

	void reset();
	void warp( const btVector3& origin );

	void preStep( btCollisionWorld* collisionWorld );
	void playerStep( btCollisionWorld* collisionWorld, btScalar dt );

	void setFallSpeed( btScalar fallSpeed );
	void setJumpSpeed( btScalar jumpSpeed );
	void setMaxJumpHeight( btScalar maxJumpHeight );
	bool canJump() const;

	void jump();

	void setGravity( btScalar gravity );
	btScalar getGravity() const;

	/// The max slope determines the maximum angle that the controller can walk up.
	/// The slope angle is measured in radians.
	void setMaxSlope( btScalar slopeRadians );
	btScalar getMaxSlope() const;

	void setUseGhostSweepTest( bool useGhostObjectSweepTest )
	{
		m_useGhostObjectSweepTest = useGhostObjectSweepTest;
	}

	bool onGround() const;
};

#endif // KINEMATIC_CHARACTER_CONTROLLER_H
