#include "physicssystem.hpp"

#include <stdexcept>

#include <osg/Group>
#include <osg/PositionAttitudeTransform>

#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>
#include <BulletCollision/CollisionShapes/btConeShape.h>
#include <BulletCollision/CollisionShapes/btSphereShape.h>
#include <BulletCollision/CollisionShapes/btStaticPlaneShape.h>
#include <BulletCollision/CollisionShapes/btCompoundShape.h>
#include <BulletCollision/CollisionDispatch/btCollisionObject.h>
#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>
#include <BulletCollision/CollisionDispatch/btDefaultCollisionConfiguration.h>
#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>
#include <BulletCollision/BroadphaseCollision/btDbvtBroadphase.h>
#include <LinearMath/btQuickprof.h>

#include <components/nifbullet/bulletshapemanager.hpp>
#include <components/nifbullet/bulletnifloader.hpp>
#include <components/resource/resourcesystem.hpp>

#include <components/esm/loadgmst.hpp>

#include <components/nifosg/particle.hpp> // FindRecIndexVisitor

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/movement.hpp"
#include "../mwmechanics/actorutil.hpp"

#include "../mwworld/esmstore.hpp"
#include "../mwworld/cellstore.hpp"

#include "../mwrender/bulletdebugdraw.hpp"

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"

#include "../mwworld/class.hpp"

#include "collisiontype.hpp"
#include "actor.hpp"
#include "convert.hpp"
#include "trace.h"

namespace MWPhysics
{

    static const float sMaxSlope = 49.0f;
    static const float sStepSizeUp = 34.0f;
    static const float sStepSizeDown = 62.0f;

    // Arbitrary number. To prevent infinite loops. They shouldn't happen but it's good to be prepared.
    static const int sMaxIterations = 8;

    // FIXME: move to a separate file
    class MovementSolver
    {
    private:
        static float getSlope(osg::Vec3f normal)
        {
            normal.normalize();
            return osg::RadiansToDegrees(std::acos(normal * osg::Vec3f(0.f, 0.f, 1.f)));
        }

        static bool stepMove(btCollisionObject *colobj, osg::Vec3f &position,
                             const osg::Vec3f &toMove, float &remainingTime, btCollisionWorld* collisionWorld)
        {
            /*
             * Slide up an incline or set of stairs.  Should be called only after a
             * collision detection otherwise unnecessary tracing will be performed.
             *
             * NOTE: with a small change this method can be used to step over an obstacle
             * of height sStepSize.
             *
             * If successful return 'true' and update 'position' to the new possible
             * location and adjust 'remainingTime'.
             *
             * If not successful return 'false'.  May fail for these reasons:
             *    - can't move directly up from current position
             *    - having moved up by between epsilon() and sStepSize, can't move forward
             *    - having moved forward by between epsilon() and toMove,
             *        = moved down between 0 and just under sStepSize but slope was too steep, or
             *        = moved the full sStepSize down (FIXME: this could be a bug)
             *
             *
             *
             * Starting position.  Obstacle or stairs with height upto sStepSize in front.
             *
             *     +--+                          +--+       |XX
             *     |  | -------> toMove          |  |    +--+XX
             *     |  |                          |  |    |XXXXX
             *     |  | +--+                     |  | +--+XXXXX
             *     |  | |XX|                     |  | |XXXXXXXX
             *     +--+ +--+                     +--+ +--------
             *    ==============================================
             */

            /*
             * Try moving up sStepSize using stepper.
             * FIXME: does not work in case there is no front obstacle but there is one above
             *
             *     +--+                         +--+
             *     |  |                         |  |
             *     |  |                         |  |       |XX
             *     |  |                         |  |    +--+XX
             *     |  |                         |  |    |XXXXX
             *     +--+ +--+                    +--+ +--+XXXXX
             *          |XX|                         |XXXXXXXX
             *          +--+                         +--------
             *    ==============================================
             */
            ActorTracer tracer, stepper;

            stepper.doTrace(colobj, position, position+osg::Vec3f(0.0f,0.0f,sStepSizeUp), collisionWorld);
            if(stepper.mFraction < std::numeric_limits<float>::epsilon())
                return false; // didn't even move the smallest representable amount
                              // (TODO: shouldn't this be larger? Why bother with such a small amount?)

            /*
             * Try moving from the elevated position using tracer.
             *
             *                          +--+  +--+
             *                          |  |  |YY|   FIXME: collision with object YY
             *                          |  |  +--+
             *                          |  |
             *     <------------------->|  |
             *          +--+            +--+
             *          |XX|      the moved amount is toMove*tracer.mFraction
             *          +--+
             *    ==============================================
             */
            tracer.doTrace(colobj, stepper.mEndPos, stepper.mEndPos + toMove, collisionWorld);
            if(tracer.mFraction < std::numeric_limits<float>::epsilon())
                return false; // didn't even move the smallest representable amount

            /*
             * Try moving back down sStepSizeDown using stepper.
             * NOTE: if there is an obstacle below (e.g. stairs), we'll be "stepping up".
             * Below diagram is the case where we "stepped over" an obstacle in front.
             *
             *                                +--+
             *                                |YY|
             *                          +--+  +--+
             *                          |  |
             *                          |  |
             *          +--+            |  |
             *          |XX|            |  |
             *          +--+            +--+
             *    ==============================================
             */
            stepper.doTrace(colobj, tracer.mEndPos, tracer.mEndPos-osg::Vec3f(0.0f,0.0f,sStepSizeDown), collisionWorld);
            if(stepper.mFraction < 1.0f && getSlope(stepper.mPlaneNormal) <= sMaxSlope)
            {
                // don't allow stepping up other actors
                if (stepper.mHitObject->getBroadphaseHandle()->m_collisionFilterGroup == CollisionType_Actor)
                    return false;
                // only step down onto semi-horizontal surfaces. don't step down onto the side of a house or a wall.
                // TODO: stepper.mPlaneNormal does not appear to be reliable - needs more testing
                // NOTE: caller's variables 'position' & 'remainingTime' are modified here
                position = stepper.mEndPos;
                remainingTime *= (1.0f-tracer.mFraction); // remaining time is proportional to remaining distance
                return true;
            }

            // moved between 0 and just under sStepSize distance but slope was too great,
            // or moved full sStepSize distance (FIXME: is this a bug?)
            return false;
        }


        ///Project a vector u on another vector v
        static inline osg::Vec3f project(const osg::Vec3f& u, const osg::Vec3f &v)
        {
            return v * (u * v);
            //            ^ dot product
        }

        ///Helper for computing the character sliding
        static inline osg::Vec3f slide(const osg::Vec3f& direction, const osg::Vec3f &planeNormal)
        {
            return direction - project(direction, planeNormal);
        }

        static inline osg::Vec3f reflect(const osg::Vec3& velocity, const osg::Vec3f& normal)
        {
            return velocity - (normal * (normal * velocity)) * 2;
            //                                  ^ dot product
        }


    public:
        static osg::Vec3f traceDown(const MWWorld::Ptr &ptr, Actor* actor, btCollisionWorld* collisionWorld, float maxHeight)
        {
            osg::Vec3f position(ptr.getRefData().getPosition().asVec3());

            ActorTracer tracer;
            tracer.findGround(actor, position, position-osg::Vec3f(0,0,maxHeight), collisionWorld);
            if(tracer.mFraction >= 1.0f)
            {
                actor->setOnGround(false);
                return position;
            }
            else
            {
                // Check if we actually found a valid spawn point (use an infinitely thin ray this time).
                // Required for some broken door destinations in Morrowind.esm, where the spawn point
                // intersects with other geometry if the actor's base is taken into account
                btVector3 from = toBullet(position);
                btVector3 to = from - btVector3(0,0,maxHeight);

                btCollisionWorld::ClosestRayResultCallback resultCallback1(from, to);
                resultCallback1.m_collisionFilterGroup = 0xff;
                resultCallback1.m_collisionFilterMask = CollisionType_World|CollisionType_HeightMap;

                collisionWorld->rayTest(from, to, resultCallback1);
                if (resultCallback1.hasHit() &&
                        ( (toOsg(resultCallback1.m_hitPointWorld) - tracer.mEndPos).length() > 30
                        || getSlope(tracer.mPlaneNormal) > sMaxSlope))
                {
                    actor->setOnGround(getSlope(toOsg(resultCallback1.m_hitNormalWorld)) <= sMaxSlope);
                    return toOsg(resultCallback1.m_hitPointWorld) + osg::Vec3f(0.f, 0.f, 1.f);
                }

                actor->setOnGround(getSlope(tracer.mPlaneNormal) <= sMaxSlope);

                return tracer.mEndPos;
            }
        }

        static osg::Vec3f move(const MWWorld::Ptr &ptr, Actor* physicActor, const osg::Vec3f &movement, float time,
                                  bool isFlying, float waterlevel, float slowFall, btCollisionWorld* collisionWorld
                                  , std::map<MWWorld::Ptr, MWWorld::Ptr>& collisionTracker
                                  , std::map<MWWorld::Ptr, MWWorld::Ptr>& standingCollisionTracker)
        {
            const ESM::Position& refpos = ptr.getRefData().getPosition();
            osg::Vec3f position(refpos.asVec3());

            // Early-out for totally static creatures
            // (Not sure if gravity should still apply?)
            if (!ptr.getClass().isMobile(ptr))
                return position;

            // Reset per-frame data
            physicActor->setWalkingOnWater(false);
            // Anything to collide with?
            if(!physicActor->getCollisionMode())
            {
                return position +  (osg::Quat(refpos.rot[0], osg::Vec3f(-1, 0, 0)) *
                                    osg::Quat(refpos.rot[2], osg::Vec3f(0, 0, -1))
                                    ) * movement * time;
            }

            btCollisionObject *colobj = physicActor->getCollisionObject();
            osg::Vec3f halfExtents = physicActor->getHalfExtents();

            // NOTE: here we don't account for the collision box translation (i.e. physicActor->getPosition() - refpos.pos).
            // That means the collision shape used for moving this actor is in a different spot than the collision shape
            // other actors are using to collide against this actor.
            // While this is strictly speaking wrong, it's needed for MW compatibility.
            position.z() += halfExtents.z();

            static const float fSwimHeightScale = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>()
                    .find("fSwimHeightScale")->getFloat();
            float swimlevel = waterlevel + halfExtents.z() - (physicActor->getRenderingHalfExtents().z() * 2 * fSwimHeightScale);

            ActorTracer tracer;
            osg::Vec3f inertia = physicActor->getInertialForce();
            osg::Vec3f velocity;

            if(position.z() < swimlevel || isFlying)
            {
                velocity = (osg::Quat(refpos.rot[0], osg::Vec3f(-1, 0, 0)) *
                            osg::Quat(refpos.rot[2], osg::Vec3f(0, 0, -1))) * movement;
            }
            else
            {
                velocity = (osg::Quat(refpos.rot[2], osg::Vec3f(0, 0, -1))) * movement;

                if (velocity.z() > 0.f)
                    inertia = velocity;
                if(!physicActor->getOnGround())
                {
                    velocity = velocity + physicActor->getInertialForce();
                }
            }

            // dead actors underwater will float to the surface
            if (ptr.getClass().getCreatureStats(ptr).isDead() && position.z() < swimlevel)
                velocity = osg::Vec3f(0,0,1) * 25;

            ptr.getClass().getMovementSettings(ptr).mPosition[2] = 0;

            // Now that we have the effective movement vector, apply wind forces to it
            if (MWBase::Environment::get().getWorld()->isInStorm())
            {
                osg::Vec3f stormDirection = MWBase::Environment::get().getWorld()->getStormDirection();
                float angleDegrees = osg::RadiansToDegrees(std::acos(stormDirection * velocity / (stormDirection.length() * velocity.length())));
                static const float fStromWalkMult = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>()
                        .find("fStromWalkMult")->getFloat();
                velocity *= 1.f-(fStromWalkMult * (angleDegrees/180.f));
            }

            osg::Vec3f origVelocity = velocity;

            osg::Vec3f newPosition = position;
            /*
             * A loop to find newPosition using tracer, if successful different from the starting position.
             * nextpos is the local variable used to find potential newPosition, using velocity and remainingTime
             * The initial velocity was set earlier (see above).
             */
            float remainingTime = time;
            for(int iterations = 0; iterations < sMaxIterations && remainingTime > 0.01f; ++iterations)
            {
                osg::Vec3f nextpos = newPosition + velocity * remainingTime;

                // If not able to fly, don't allow to swim up into the air
                if(newPosition.z() < swimlevel &&
                   !isFlying &&  // can't fly
                   nextpos.z() > swimlevel &&     // but about to go above water
                   newPosition.z() <= swimlevel)
                {
                    const osg::Vec3f down(0,0,-1);
                    float movelen = velocity.normalize();
                    osg::Vec3f reflectdir = reflect(velocity, down);
                    reflectdir.normalize();
                    velocity = slide(reflectdir, down)*movelen;
                    // NOTE: remainingTime is unchanged before the loop continues
                    continue; // velocity updated, calculate nextpos again
                }

                if((newPosition - nextpos).length2() > 0.0001)
                {
                    // trace to where character would go if there were no obstructions
                    tracer.doTrace(colobj, newPosition, nextpos, collisionWorld);

                    // check for obstructions
                    if(tracer.mFraction >= 1.0f)
                    {
                        newPosition = tracer.mEndPos; // ok to move, so set newPosition
                        break;
                    }
                    else
                    {
                        const btCollisionObject* standingOn = tracer.mHitObject;
                        const PtrHolder* ptrHolder = static_cast<const PtrHolder*>(standingOn->getUserPointer());
                        if (ptrHolder)
                            collisionTracker[ptr] = ptrHolder->getPtr();
                    }
                }
                else
                {
                    // The current position and next position are nearly the same, so just exit.
                    // Note: Bullet can trigger an assert in debug modes if the positions
                    // are the same, since that causes it to attempt to normalize a zero
                    // length vector (which can also happen with nearly identical vectors, since
                    // precision can be lost due to any math Bullet does internally). Since we
                    // aren't performing any collision detection, we want to reject the next
                    // position, so that we don't slowly move inside another object.
                    break;
                }


                osg::Vec3f oldPosition = newPosition;
                // We hit something. Try to step up onto it. (NOTE: stepMove does not allow stepping over)
                // NOTE: stepMove modifies newPosition if successful
                bool result = stepMove(colobj, newPosition, velocity*remainingTime, remainingTime, collisionWorld);
                if (!result) // to make sure the maximum stepping distance isn't framerate-dependent or movement-speed dependent
                {
                    osg::Vec3f normalizedVelocity = velocity;
                    normalizedVelocity.normalize();
                    result = stepMove(colobj, newPosition, normalizedVelocity*10.f, remainingTime, collisionWorld);
                }
                if(result)
                {
                    // don't let pure water creatures move out of water after stepMove
                    if (ptr.getClass().isPureWaterCreature(ptr)
                            && newPosition.z() + halfExtents.z() > waterlevel)
                        newPosition = oldPosition;
                }
                else
                {
                    // Can't move this way, try to find another spot along the plane
                    osg::Vec3f direction = velocity;
                    float movelen = direction.normalize();
                    osg::Vec3f reflectdir = reflect(velocity, tracer.mPlaneNormal);
                    reflectdir.normalize();

                    osg::Vec3f newVelocity = slide(reflectdir, tracer.mPlaneNormal)*movelen;
                    if ((newVelocity-velocity).length2() < 0.01)
                        break;
                    if ((velocity * origVelocity) <= 0.f)
                        break; // ^ dot product

                    velocity = newVelocity;

                    // Do not allow sliding upward if there is gravity. Stepping will have taken
                    // care of that.
                    if(!(newPosition.z() < swimlevel || isFlying))
                        velocity.z() = std::min(velocity.z(), 0.0f);
                }
            }

            bool isOnGround = false;
            if (!(inertia.z() > 0.f) && !(newPosition.z() < swimlevel))
            {
                osg::Vec3f from = newPosition;
                osg::Vec3f to = newPosition - (physicActor->getOnGround() ?
                             osg::Vec3f(0,0,sStepSizeDown+2.f) : osg::Vec3f(0,0,2.f));
                tracer.doTrace(colobj, from, to, collisionWorld);
                if(tracer.mFraction < 1.0f && getSlope(tracer.mPlaneNormal) <= sMaxSlope
                        && tracer.mHitObject->getBroadphaseHandle()->m_collisionFilterGroup != CollisionType_Actor)
                {
                    const btCollisionObject* standingOn = tracer.mHitObject;
                    const PtrHolder* ptrHolder = static_cast<PtrHolder*>(standingOn->getUserPointer());
                    if (ptrHolder)
                        standingCollisionTracker[ptr] = ptrHolder->getPtr();

                    if (standingOn->getBroadphaseHandle()->m_collisionFilterGroup == CollisionType_Water)
                        physicActor->setWalkingOnWater(true);
                    if (!isFlying)
                        newPosition.z() = tracer.mEndPos.z() + 1.0f;

                    isOnGround = true;
                }
                else
                {
                    // standing on actors is not allowed (see above).
                    // in addition to that, apply a sliding effect away from the center of the actor,
                    // so that we do not stay suspended in air indefinitely.
                    if (tracer.mFraction < 1.0f && tracer.mHitObject->getBroadphaseHandle()->m_collisionFilterGroup == CollisionType_Actor)
                    {
                        if (osg::Vec3f(velocity.x(), velocity.y(), 0).length2() < 100.f*100.f)
                        {
                            btVector3 aabbMin, aabbMax;
                            tracer.mHitObject->getCollisionShape()->getAabb(tracer.mHitObject->getWorldTransform(), aabbMin, aabbMax);
                            btVector3 center = (aabbMin + aabbMax) / 2.f;
                            inertia = osg::Vec3f(position.x() - center.x(), position.y() - center.y(), 0);
                            inertia.normalize();
                            inertia *= 100;
                        }
                    }

                    isOnGround = false;
                }
            }

            if(isOnGround || newPosition.z() < swimlevel || isFlying)
                physicActor->setInertialForce(osg::Vec3f(0.f, 0.f, 0.f));
            else
            {
                inertia.z() += time * -627.2f;
                if (inertia.z() < 0)
                    inertia.z() *= slowFall;
                physicActor->setInertialForce(inertia);
            }
            physicActor->setOnGround(isOnGround);

            newPosition.z() -= halfExtents.z(); // remove what was added at the beginning
            return newPosition;
        }
    };


    // ---------------------------------------------------------------

    class HeightField
    {
    public:
        HeightField(const float* heights, int x, int y, float triSize, float sqrtVerts)
        {
            // find the minimum and maximum heights (needed for bullet)
            float minh = heights[0];
            float maxh = heights[0];
            for(int i = 1;i < sqrtVerts*sqrtVerts;++i)
            {
                float h = heights[i];
                if(h > maxh) maxh = h;
                if(h < minh) minh = h;
            }

            mShape = new btHeightfieldTerrainShape(
                sqrtVerts, sqrtVerts, heights, 1,
                minh, maxh, 2,
                PHY_FLOAT, true
            );
            mShape->setUseDiamondSubdivision(true);
            mShape->setLocalScaling(btVector3(triSize, triSize, 1));

            btTransform transform(btQuaternion::getIdentity(),
                                  btVector3((x+0.5f) * triSize * (sqrtVerts-1),
                                            (y+0.5f) * triSize * (sqrtVerts-1),
                                            (maxh+minh)*0.5f));

            mCollisionObject = new btCollisionObject;
            mCollisionObject->setCollisionShape(mShape);
            mCollisionObject->setWorldTransform(transform);
        }
        ~HeightField()
        {
            delete mCollisionObject;
            delete mShape;
        }
        btCollisionObject* getCollisionObject()
        {
            return mCollisionObject;
        }

    private:
        btHeightfieldTerrainShape* mShape;
        btCollisionObject* mCollisionObject;
    };

    // --------------------------------------------------------------

    class Object : public PtrHolder
    {
    public:
        Object(const MWWorld::Ptr& ptr, osg::ref_ptr<NifBullet::BulletShapeInstance> shapeInstance)
            : mShapeInstance(shapeInstance)
        {
            mPtr = ptr;

            mCollisionObject.reset(new btCollisionObject);
            mCollisionObject->setCollisionShape(shapeInstance->getCollisionShape());

            mCollisionObject->setUserPointer(static_cast<PtrHolder*>(this));

            setScale(ptr.getCellRef().getScale());
            setRotation(toBullet(ptr.getRefData().getBaseNode()->getAttitude()));
            const float* pos = ptr.getRefData().getPosition().pos;
            setOrigin(btVector3(pos[0], pos[1], pos[2]));
        }

        void setScale(float scale)
        {
            mShapeInstance->getCollisionShape()->setLocalScaling(btVector3(scale,scale,scale));
        }

        void setRotation(const btQuaternion& quat)
        {
            mCollisionObject->getWorldTransform().setRotation(quat);
        }

        void setOrigin(const btVector3& vec)
        {
            mCollisionObject->getWorldTransform().setOrigin(vec);
        }

        btCollisionObject* getCollisionObject()
        {
            return mCollisionObject.get();
        }

        void animateCollisionShapes(btCollisionWorld* collisionWorld)
        {
            if (mShapeInstance->mAnimatedShapes.empty())
                return;

            assert (mShapeInstance->getCollisionShape()->isCompound());

            btCompoundShape* compound = dynamic_cast<btCompoundShape*>(mShapeInstance->getCollisionShape());

            for (std::map<int, int>::const_iterator it = mShapeInstance->mAnimatedShapes.begin(); it != mShapeInstance->mAnimatedShapes.end(); ++it)
            {
                int recIndex = it->first;
                int shapeIndex = it->second;

                NifOsg::FindRecIndexVisitor visitor(recIndex);
                mPtr.getRefData().getBaseNode()->accept(visitor);
                if (!visitor.mFound)
                {
                    std::cerr << "animateCollisionShapes: Can't find node " << recIndex << std::endl;
                    return;
                }

                osg::NodePath path = visitor.mFoundPath;
                path.erase(path.begin());
                osg::Matrixf matrix = osg::computeLocalToWorld(path);
                osg::Vec3f scale = matrix.getScale();
                matrix.orthoNormalize(matrix);

                btTransform transform;
                transform.setOrigin(toBullet(matrix.getTrans()) * compound->getLocalScaling());
                for (int i=0; i<3; ++i)
                    for (int j=0; j<3; ++j)
                        transform.getBasis()[i][j] = matrix(j,i); // NB column/row major difference

                compound->getChildShape(shapeIndex)->setLocalScaling(compound->getLocalScaling() * toBullet(scale));
                compound->updateChildTransform(shapeIndex, transform);
            }

            collisionWorld->updateSingleAabb(mCollisionObject.get());
        }

    private:
        std::auto_ptr<btCollisionObject> mCollisionObject;
        osg::ref_ptr<NifBullet::BulletShapeInstance> mShapeInstance;
    };

    // ---------------------------------------------------------------

    PhysicsSystem::PhysicsSystem(Resource::ResourceSystem* resourceSystem, osg::ref_ptr<osg::Group> parentNode)
        : mShapeManager(new NifBullet::BulletShapeManager(resourceSystem->getVFS()))
        , mDebugDrawEnabled(false)
        , mTimeAccum(0.0f)
        , mWaterHeight(0)
        , mWaterEnabled(false)
        , mParentNode(parentNode)
    {
        mCollisionConfiguration = new btDefaultCollisionConfiguration();
        mDispatcher = new btCollisionDispatcher(mCollisionConfiguration);
        mBroadphase = new btDbvtBroadphase();

        mCollisionWorld = new btCollisionWorld(mDispatcher, mBroadphase, mCollisionConfiguration);

        // Don't update AABBs of all objects every frame. Most objects in MW are static, so we don't need this.
        // Should a "static" object ever be moved, we have to update its AABB manually using DynamicsWorld::updateSingleAabb.
        mCollisionWorld->setForceUpdateAllAabbs(false);
    }

    PhysicsSystem::~PhysicsSystem()
    {
        if (mWaterCollisionObject.get())
            mCollisionWorld->removeCollisionObject(mWaterCollisionObject.get());

        for (HeightFieldMap::iterator it = mHeightFields.begin(); it != mHeightFields.end(); ++it)
        {
            mCollisionWorld->removeCollisionObject(it->second->getCollisionObject());
            delete it->second;
        }

        for (ObjectMap::iterator it = mObjects.begin(); it != mObjects.end(); ++it)
        {
            mCollisionWorld->removeCollisionObject(it->second->getCollisionObject());
            delete it->second;
        }

        for (ActorMap::iterator it = mActors.begin(); it != mActors.end(); ++it)
        {
            delete it->second;
        }

        delete mCollisionWorld;
        delete mCollisionConfiguration;
        delete mDispatcher;
        delete mBroadphase;
    }

    bool PhysicsSystem::toggleDebugRendering()
    {
        mDebugDrawEnabled = !mDebugDrawEnabled;

        if (mDebugDrawEnabled && !mDebugDrawer.get())
        {
            mDebugDrawer.reset(new MWRender::DebugDrawer(mParentNode, mCollisionWorld));
            mCollisionWorld->setDebugDrawer(mDebugDrawer.get());
            mDebugDrawer->setDebugMode(mDebugDrawEnabled);
        }
        else if (mDebugDrawer.get())
            mDebugDrawer->setDebugMode(mDebugDrawEnabled);
        return mDebugDrawEnabled;
    }

    class DeepestNotMeContactTestResultCallback : public btCollisionWorld::ContactResultCallback
    {
        const btCollisionObject* mMe;

        // Store the real origin, since the shape's origin is its center
        btVector3 mOrigin;

    public:
        const btCollisionObject *mObject;
        btVector3 mContactPoint;
        btScalar mLeastDistSqr;

        DeepestNotMeContactTestResultCallback(const btCollisionObject* me, const btVector3 &origin)
          : mMe(me), mOrigin(origin), mObject(NULL), mContactPoint(0,0,0),
            mLeastDistSqr(std::numeric_limits<float>::max())
        { }

#if BT_BULLET_VERSION >= 281
        virtual btScalar addSingleResult(btManifoldPoint& cp,
                                         const btCollisionObjectWrapper* col0Wrap,int partId0,int index0,
                                         const btCollisionObjectWrapper* col1Wrap,int partId1,int index1)
        {
            const btCollisionObject* collisionObject = col1Wrap->m_collisionObject;
#else
        virtual btScalar addSingleResult(btManifoldPoint& cp, const btCollisionObject* col0, int partId0, int index0,
                                         const btCollisionObject* col1, int partId1, int index1)
        {
            const btCollisionObject* collisionObject = col1;
#endif
            if (collisionObject != mMe)
            {
                btScalar distsqr = mOrigin.distance2(cp.getPositionWorldOnA());
                if(!mObject || distsqr < mLeastDistSqr)
                {
                    mObject = collisionObject;
                    mLeastDistSqr = distsqr;
                    mContactPoint = cp.getPositionWorldOnA();
                }
            }

            return 0.f;
        }
    };

    std::pair<MWWorld::Ptr, osg::Vec3f> PhysicsSystem::getHitContact(const MWWorld::Ptr& actor,
                                                                     const osg::Vec3f &origin,
                                                                     const osg::Quat &orient,
                                                                      float queryDistance)
    {
        const MWWorld::Store<ESM::GameSetting> &store = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

        btConeShape shape (osg::DegreesToRadians(store.find("fCombatAngleXY")->getFloat()/2.0f), queryDistance);
        shape.setLocalScaling(btVector3(1, 1, osg::DegreesToRadians(store.find("fCombatAngleZ")->getFloat()/2.0f) /
                                              shape.getRadius()));

        // The shape origin is its center, so we have to move it forward by half the length. The
        // real origin will be provided to getFilteredContact to find the closest.
        osg::Vec3f center = origin + (orient * osg::Vec3f(0.0f, queryDistance*0.5f, 0.0f));

        btCollisionObject object;
        object.setCollisionShape(&shape);
        object.setWorldTransform(btTransform(toBullet(orient), toBullet(center)));

        const btCollisionObject* me = NULL;
        Actor* physactor = getActor(actor);
        if (physactor)
            me = physactor->getCollisionObject();

        DeepestNotMeContactTestResultCallback resultCallback(me, toBullet(origin));
        resultCallback.m_collisionFilterGroup = CollisionType_Actor;
        resultCallback.m_collisionFilterMask = CollisionType_World | CollisionType_HeightMap | CollisionType_Actor;
        mCollisionWorld->contactTest(&object, resultCallback);

        if (resultCallback.mObject)
        {
            const PtrHolder* holder = static_cast<const PtrHolder*>(resultCallback.mObject->getUserPointer());
            if (holder)
                return std::make_pair(holder->getPtr(), toOsg(resultCallback.mContactPoint));
        }
        return std::make_pair(MWWorld::Ptr(), osg::Vec3f());
    }

    class ClosestNotMeRayResultCallback : public btCollisionWorld::ClosestRayResultCallback
    {
    public:
        ClosestNotMeRayResultCallback(const btCollisionObject* me, const btVector3& from, const btVector3& to)
            : btCollisionWorld::ClosestRayResultCallback(from, to)
            , mMe(me)
        {
        }

        virtual btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult,bool normalInWorldSpace)
        {
            if (rayResult.m_collisionObject == mMe)
                return 1.f;
            return btCollisionWorld::ClosestRayResultCallback::addSingleResult(rayResult, normalInWorldSpace);
        }
    private:
        const btCollisionObject* mMe;
    };

    PhysicsSystem::RayResult PhysicsSystem::castRay(const osg::Vec3f &from, const osg::Vec3f &to, MWWorld::Ptr ignore, int mask, int group)
    {
        btVector3 btFrom = toBullet(from);
        btVector3 btTo = toBullet(to);

        const btCollisionObject* me = NULL;
        if (!ignore.isEmpty())
        {
            Actor* actor = getActor(ignore);
            if (actor)
                me = actor->getCollisionObject();
        }

        ClosestNotMeRayResultCallback resultCallback(me, btFrom, btTo);
        resultCallback.m_collisionFilterGroup = group;
        resultCallback.m_collisionFilterMask = mask;

        mCollisionWorld->rayTest(btFrom, btTo, resultCallback);

        RayResult result;
        result.mHit = resultCallback.hasHit();
        if (resultCallback.hasHit())
        {
            result.mHitPos = toOsg(resultCallback.m_hitPointWorld);
            result.mHitNormal = toOsg(resultCallback.m_hitNormalWorld);
            if (PtrHolder* ptrHolder = static_cast<PtrHolder*>(resultCallback.m_collisionObject->getUserPointer()))
                result.mHitObject = ptrHolder->getPtr();
        }
        return result;
    }

    PhysicsSystem::RayResult PhysicsSystem::castSphere(const osg::Vec3f &from, const osg::Vec3f &to, float radius)
    {
        btCollisionWorld::ClosestConvexResultCallback callback(toBullet(from), toBullet(to));
        callback.m_collisionFilterGroup = 0xff;
        callback.m_collisionFilterMask = CollisionType_World|CollisionType_HeightMap;

        btSphereShape shape(radius);
        const btQuaternion btrot = btQuaternion::getIdentity();

        btTransform from_ (btrot, toBullet(from));
        btTransform to_ (btrot, toBullet(to));

        mCollisionWorld->convexSweepTest(&shape, from_, to_, callback);

        RayResult result;
        result.mHit = callback.hasHit();
        if (result.mHit)
        {
            result.mHitPos = toOsg(callback.m_hitPointWorld);
            result.mHitNormal = toOsg(callback.m_hitNormalWorld);
        }
        return result;
    }

    bool PhysicsSystem::getLineOfSight(const MWWorld::Ptr &actor1, const MWWorld::Ptr &actor2)
    {
        Actor* physactor1 = getActor(actor1);
        Actor* physactor2 = getActor(actor2);

        if (!physactor1 || !physactor2)
            return false;

        osg::Vec3f pos1 (physactor1->getPosition() + osg::Vec3f(0,0,physactor1->getHalfExtents().z() * 0.8)); // eye level
        osg::Vec3f pos2 (physactor2->getPosition() + osg::Vec3f(0,0,physactor2->getHalfExtents().z() * 0.8));

        RayResult result = castRay(pos1, pos2, MWWorld::Ptr(), CollisionType_World|CollisionType_HeightMap);

        return !result.mHit;
    }

    // physactor->getOnGround() is not a reliable indicator of whether the actor
    // is on the ground (defaults to false, which means code blocks such as
    // CharacterController::update() may falsely detect "falling").
    //
    // Also, collisions can move z position slightly off zero, giving a false
    // indication. In order to reduce false detection of jumping, small distance
    // below the actor is detected and ignored. A value of 1.5 is used here, but
    // something larger may be more suitable.  This change should resolve Bug#1271.
    //
    // TODO: There might be better places to update PhysicActor::mOnGround.
    bool PhysicsSystem::isOnGround(const MWWorld::Ptr &actor)
    {
        Actor* physactor = getActor(actor);
        if(!physactor)
            return false;
        if(physactor->getOnGround())
            return true;
        else
        {
            osg::Vec3f pos(actor.getRefData().getPosition().asVec3());

            ActorTracer tracer;
            // a small distance above collision object is considered "on ground"
            tracer.findGround(physactor,
                              pos,
                              pos - osg::Vec3f(0, 0, 1.5f), // trace a small amount down
                              mCollisionWorld);
            if(tracer.mFraction < 1.0f) // collision, must be close to something below
            {
                physactor->setOnGround(true);
                return true;
            }
            else
                return false;
        }
    }

    osg::Vec3f PhysicsSystem::getHalfExtents(const MWWorld::Ptr &actor) const
    {
        const Actor* physactor = getActor(actor);
        if (physactor)
            return physactor->getHalfExtents();
        else
            return osg::Vec3f();
    }

    osg::Vec3f PhysicsSystem::getRenderingHalfExtents(const MWWorld::Ptr &actor) const
    {
        const Actor* physactor = getActor(actor);
        if (physactor)
            return physactor->getRenderingHalfExtents();
        else
            return osg::Vec3f();
    }

    osg::Vec3f PhysicsSystem::getPosition(const MWWorld::Ptr &actor) const
    {
        const Actor* physactor = getActor(actor);
        if (physactor)
            return physactor->getPosition();
        else
            return osg::Vec3f();
    }

    class ContactTestResultCallback : public btCollisionWorld::ContactResultCallback
    {
    public:
        std::vector<MWWorld::Ptr> mResult;

#if BT_BULLET_VERSION >= 281
        virtual btScalar addSingleResult(btManifoldPoint& cp,
                                         const btCollisionObjectWrapper* col0Wrap,int partId0,int index0,
                                         const btCollisionObjectWrapper* col1Wrap,int partId1,int index1)
        {
            const btCollisionObject* collisionObject = col0Wrap->m_collisionObject;
#else
        virtual btScalar addSingleResult(btManifoldPoint& cp, const btCollisionObject* col0, int partId0, int index0,
                                         const btCollisionObject* col1, int partId1, int index1)
        {
            const btCollisionObject* collisionObject = col0;
#endif
            const PtrHolder* holder = static_cast<const PtrHolder*>(collisionObject->getUserPointer());
            if (holder)
                mResult.push_back(holder->getPtr());
            return 0.f;
        }
    };

    std::vector<MWWorld::Ptr> PhysicsSystem::getCollisions(const MWWorld::Ptr &ptr, int collisionGroup, int collisionMask)
    {
        btCollisionObject* me = NULL;

        ObjectMap::iterator found = mObjects.find(ptr);
        if (found != mObjects.end())
            me = found->second->getCollisionObject();
        else
            return std::vector<MWWorld::Ptr>();

        ContactTestResultCallback resultCallback;
        resultCallback.m_collisionFilterGroup = collisionGroup;
        resultCallback.m_collisionFilterMask = collisionMask;
        mCollisionWorld->contactTest(me, resultCallback);
        return resultCallback.mResult;
    }

    osg::Vec3f PhysicsSystem::traceDown(const MWWorld::Ptr &ptr, float maxHeight)
    {
        ActorMap::iterator found = mActors.find(ptr);
        if (found ==  mActors.end())
            return ptr.getRefData().getPosition().asVec3();
        else
            return MovementSolver::traceDown(ptr, found->second, mCollisionWorld, maxHeight);
    }

    void PhysicsSystem::addHeightField (const float* heights, int x, int y, float triSize, float sqrtVerts)
    {
        HeightField *heightfield = new HeightField(heights, x, y, triSize, sqrtVerts);
        mHeightFields[std::make_pair(x,y)] = heightfield;

        mCollisionWorld->addCollisionObject(heightfield->getCollisionObject(), CollisionType_HeightMap,
            CollisionType_Actor|CollisionType_Projectile);
    }

    void PhysicsSystem::removeHeightField (int x, int y)
    {
        HeightFieldMap::iterator heightfield = mHeightFields.find(std::make_pair(x,y));
        if(heightfield != mHeightFields.end())
        {
            mCollisionWorld->removeCollisionObject(heightfield->second->getCollisionObject());
            delete heightfield->second;
            mHeightFields.erase(heightfield);
        }
    }

    void PhysicsSystem::addObject (const MWWorld::Ptr& ptr, const std::string& mesh)
    {
        osg::ref_ptr<NifBullet::BulletShapeInstance> shapeInstance = mShapeManager->createInstance(mesh);
        if (!shapeInstance->getCollisionShape())
            return;

        Object *obj = new Object(ptr, shapeInstance);
        mObjects.insert(std::make_pair(ptr, obj));

        mCollisionWorld->addCollisionObject(obj->getCollisionObject(), CollisionType_World,
                                           CollisionType_Actor|CollisionType_HeightMap|CollisionType_Projectile);
    }

    void PhysicsSystem::remove(const MWWorld::Ptr &ptr)
    {
        ObjectMap::iterator found = mObjects.find(ptr);
        if (found != mObjects.end())
        {
            mCollisionWorld->removeCollisionObject(found->second->getCollisionObject());
            delete found->second;
            mObjects.erase(found);
        }

        ActorMap::iterator foundActor = mActors.find(ptr);
        if (foundActor != mActors.end())
        {
            delete foundActor->second;
            mActors.erase(foundActor);
        }
    }

    void PhysicsSystem::updateCollisionMapPtr(CollisionMap& map, const MWWorld::Ptr &old, const MWWorld::Ptr &updated)
    {
        CollisionMap::iterator found = map.find(old);
        if (found != map.end())
        {
            map[updated] = found->second;
            map.erase(found);
        }

        for (CollisionMap::iterator it = map.begin(); it != map.end(); ++it)
        {
            if (it->second == old)
                it->second = updated;
        }
    }

    void PhysicsSystem::updatePtr(const MWWorld::Ptr &old, const MWWorld::Ptr &updated)
    {
        ObjectMap::iterator found = mObjects.find(old);
        if (found != mObjects.end())
        {
            Object* obj = found->second;
            obj->updatePtr(updated);
            mObjects.erase(found);
            mObjects.insert(std::make_pair(updated, obj));
        }

        ActorMap::iterator foundActor = mActors.find(old);
        if (foundActor != mActors.end())
        {
            Actor* actor = foundActor->second;
            actor->updatePtr(updated);
            mActors.erase(foundActor);
            mActors.insert(std::make_pair(updated, actor));
        }

        updateCollisionMapPtr(mCollisions, old, updated);
        updateCollisionMapPtr(mStandingCollisions, old, updated);
    }

    Actor *PhysicsSystem::getActor(const MWWorld::Ptr &ptr)
    {
        ActorMap::iterator found = mActors.find(ptr);
        if (found != mActors.end())
            return found->second;
        return NULL;
    }

    const Actor *PhysicsSystem::getActor(const MWWorld::Ptr &ptr) const
    {
        ActorMap::const_iterator found = mActors.find(ptr);
        if (found != mActors.end())
            return found->second;
        return NULL;
    }

    void PhysicsSystem::updateScale(const MWWorld::Ptr &ptr)
    {
        ObjectMap::iterator found = mObjects.find(ptr);
        float scale = ptr.getCellRef().getScale();
        if (found != mObjects.end())
        {
            found->second->setScale(scale);
            mCollisionWorld->updateSingleAabb(found->second->getCollisionObject());
            return;
        }
        ActorMap::iterator foundActor = mActors.find(ptr);
        if (foundActor != mActors.end())
        {
            foundActor->second->updateScale();
            mCollisionWorld->updateSingleAabb(foundActor->second->getCollisionObject());
            return;
        }
    }

    void PhysicsSystem::updateRotation(const MWWorld::Ptr &ptr)
    {
        ObjectMap::iterator found = mObjects.find(ptr);
        if (found != mObjects.end())
        {
            found->second->setRotation(toBullet(ptr.getRefData().getBaseNode()->getAttitude()));
            mCollisionWorld->updateSingleAabb(found->second->getCollisionObject());
            return;
        }
        ActorMap::iterator foundActor = mActors.find(ptr);
        if (foundActor != mActors.end())
        {
            foundActor->second->updateRotation();
            mCollisionWorld->updateSingleAabb(foundActor->second->getCollisionObject());
            return;
        }
    }

    void PhysicsSystem::updatePosition(const MWWorld::Ptr &ptr)
    {
        ObjectMap::iterator found = mObjects.find(ptr);
        if (found != mObjects.end())
        {
            found->second->setOrigin(toBullet(ptr.getRefData().getPosition().asVec3()));
            mCollisionWorld->updateSingleAabb(found->second->getCollisionObject());
            return;
        }
        ActorMap::iterator foundActor = mActors.find(ptr);
        if (foundActor != mActors.end())
        {
            foundActor->second->updatePosition();
            mCollisionWorld->updateSingleAabb(foundActor->second->getCollisionObject());
            return;
        }
    }

    void PhysicsSystem::addActor (const MWWorld::Ptr& ptr, const std::string& mesh)
    {
        osg::ref_ptr<NifBullet::BulletShapeInstance> shapeInstance = mShapeManager->createInstance(mesh);

        Actor* actor = new Actor(ptr, shapeInstance, mCollisionWorld);
        mActors.insert(std::make_pair(ptr, actor));
    }

    bool PhysicsSystem::toggleCollisionMode()
    {
        ActorMap::iterator found = mActors.find(MWMechanics::getPlayer());
        if (found != mActors.end())
        {
            bool cmode = found->second->getCollisionMode();
            cmode = !cmode;
            found->second->enableCollisionMode(cmode);
            return cmode;
        }

        return false;
    }

    void PhysicsSystem::queueObjectMovement(const MWWorld::Ptr &ptr, const osg::Vec3f &movement)
    {
        PtrVelocityList::iterator iter = mMovementQueue.begin();
        for(;iter != mMovementQueue.end();++iter)
        {
            if(iter->first == ptr)
            {
                iter->second = movement;
                return;
            }
        }

        mMovementQueue.push_back(std::make_pair(ptr, movement));
    }

    void PhysicsSystem::clearQueuedMovement()
    {
        mMovementQueue.clear();
        mCollisions.clear();
        mStandingCollisions.clear();
    }

    const PtrVelocityList& PhysicsSystem::applyQueuedMovement(float dt)
    {
        mMovementResults.clear();

        mTimeAccum += dt;
        if(mTimeAccum >= 1.0f/60.0f)
        {
            // Collision events should be available on every frame
            mCollisions.clear();
            mStandingCollisions.clear();

            const MWBase::World *world = MWBase::Environment::get().getWorld();
            PtrVelocityList::iterator iter = mMovementQueue.begin();
            for(;iter != mMovementQueue.end();++iter)
            {
                float waterlevel = -std::numeric_limits<float>::max();
                const MWWorld::CellStore *cell = iter->first.getCell();
                if(cell->getCell()->hasWater())
                    waterlevel = cell->getWaterLevel();

                float oldHeight = iter->first.getRefData().getPosition().pos[2];

                const MWMechanics::MagicEffects& effects = iter->first.getClass().getCreatureStats(iter->first).getMagicEffects();

                bool waterCollision = false;
                if (effects.get(ESM::MagicEffect::WaterWalking).getMagnitude()
                        && cell->getCell()->hasWater()
                        && !world->isUnderwater(iter->first.getCell(),
                                               osg::Vec3f(iter->first.getRefData().getPosition().asVec3())))
                    waterCollision = true;

                ActorMap::iterator foundActor = mActors.find(iter->first);
                if (foundActor == mActors.end()) // actor was already removed from the scene
                    continue;
                Actor* physicActor = foundActor->second;
                physicActor->setCanWaterWalk(waterCollision);

                // Slow fall reduces fall speed by a factor of (effect magnitude / 200)
                float slowFall = 1.f - std::max(0.f, std::min(1.f, effects.get(ESM::MagicEffect::SlowFall).getMagnitude() * 0.005f));

                osg::Vec3f newpos = MovementSolver::move(iter->first, physicActor, iter->second, mTimeAccum,
                                                            world->isFlying(iter->first),
                                                            waterlevel, slowFall, mCollisionWorld, mCollisions, mStandingCollisions);

                float heightDiff = newpos.z() - oldHeight;

                if (heightDiff < 0)
                    iter->first.getClass().getCreatureStats(iter->first).addToFallHeight(-heightDiff);

                mMovementResults.push_back(std::make_pair(iter->first, newpos));
            }

            mTimeAccum = 0.0f;
        }
        mMovementQueue.clear();

        return mMovementResults;
    }

    void PhysicsSystem::stepSimulation(float dt)
    {
        for (ObjectMap::iterator it = mObjects.begin(); it != mObjects.end(); ++it)
            it->second->animateCollisionShapes(mCollisionWorld);

        CProfileManager::Reset();
        CProfileManager::Increment_Frame_Counter();
    }

    void PhysicsSystem::debugDraw()
    {
        if (mDebugDrawer.get())
            mDebugDrawer->step();
    }

    bool PhysicsSystem::isActorStandingOn(const MWWorld::Ptr &actor, const MWWorld::Ptr &object) const
    {
        for (CollisionMap::const_iterator it = mStandingCollisions.begin(); it != mStandingCollisions.end(); ++it)
        {
            if (it->first == actor && it->second == object)
                return true;
        }
        return false;
    }

    void PhysicsSystem::getActorsStandingOn(const MWWorld::Ptr &object, std::vector<MWWorld::Ptr> &out) const
    {
        for (CollisionMap::const_iterator it = mStandingCollisions.begin(); it != mStandingCollisions.end(); ++it)
        {
            if (it->second == object)
                out.push_back(it->first);
        }
    }

    bool PhysicsSystem::isActorCollidingWith(const MWWorld::Ptr &actor, const MWWorld::Ptr &object) const
    {
        for (CollisionMap::const_iterator it = mCollisions.begin(); it != mCollisions.end(); ++it)
        {
            if (it->first == actor && it->second == object)
                return true;
        }
        return false;
    }

    void PhysicsSystem::getActorsCollidingWith(const MWWorld::Ptr &object, std::vector<MWWorld::Ptr> &out) const
    {
        for (CollisionMap::const_iterator it = mCollisions.begin(); it != mCollisions.end(); ++it)
        {
            if (it->second == object)
                out.push_back(it->first);
        }
    }

    void PhysicsSystem::disableWater()
    {
        if (mWaterEnabled)
        {
            mWaterEnabled = false;
            updateWater();
        }
    }

    void PhysicsSystem::enableWater(float height)
    {
        if (!mWaterEnabled || mWaterHeight != height)
        {
            mWaterEnabled = true;
            mWaterHeight = height;
            updateWater();
        }
    }

    void PhysicsSystem::setWaterHeight(float height)
    {
        if (mWaterHeight != height)
        {
            mWaterHeight = height;
            updateWater();
        }
    }

    void PhysicsSystem::updateWater()
    {
        if (mWaterCollisionObject.get())
        {
            mCollisionWorld->removeCollisionObject(mWaterCollisionObject.get());
        }

        if (!mWaterEnabled)
            return;

        mWaterCollisionObject.reset(new btCollisionObject());
        mWaterCollisionShape.reset(new btStaticPlaneShape(btVector3(0,0,1), mWaterHeight));
        mWaterCollisionObject->setCollisionShape(mWaterCollisionShape.get());
        mCollisionWorld->addCollisionObject(mWaterCollisionObject.get(), CollisionType_Water,
                                                    CollisionType_Actor);
    }
}
