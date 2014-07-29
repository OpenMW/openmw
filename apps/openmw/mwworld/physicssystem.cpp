#include "physicssystem.hpp"

#include <stdexcept>

#include <OgreRoot.h>
#include <OgreRenderWindow.h>
#include <OgreSceneManager.h>
#include <OgreViewport.h>
#include <OgreCamera.h>
#include <OgreTextureManager.h>
#include <OgreSceneNode.h>

#include <openengine/bullet/trace.h>
#include <openengine/bullet/physic.hpp>
#include <openengine/bullet/BtOgreExtras.h>
#include <openengine/ogre/renderer.hpp>

#include <components/nifbullet/bulletnifloader.hpp>

#include <components/esm/loadgmst.hpp>

#include "../mwbase/world.hpp" // FIXME
#include "../mwbase/environment.hpp"

#include "../mwmechanics/creaturestats.hpp"

#include "../mwworld/esmstore.hpp"
#include "../mwworld/cellstore.hpp"

#include "../apps/openmw/mwrender/animation.hpp"
#include "../apps/openmw/mwbase/world.hpp"
#include "../apps/openmw/mwbase/environment.hpp"

#include "ptr.hpp"
#include "class.hpp"

using namespace Ogre;

namespace
{

void animateCollisionShapes (std::map<OEngine::Physic::RigidBody*, OEngine::Physic::AnimatedShapeInstance>& map)
{
    for (std::map<OEngine::Physic::RigidBody*, OEngine::Physic::AnimatedShapeInstance>::iterator it = map.begin();
         it != map.end(); ++it)
    {
        MWWorld::Ptr ptr = MWBase::Environment::get().getWorld()->searchPtrViaHandle(it->first->mName);
        if (ptr.isEmpty()) // Shouldn't happen
            throw std::runtime_error("can't find Ptr");

        MWRender::Animation* animation = MWBase::Environment::get().getWorld()->getAnimation(ptr);
        if (!animation) // Shouldn't happen either, since keyframe-controlled objects are not batched in StaticGeometry
            throw std::runtime_error("can't find Animation for " + ptr.getCellRef().getRefId());

        OEngine::Physic::AnimatedShapeInstance& instance = it->second;

        std::map<std::string, int>& shapes = instance.mAnimatedShapes;
        for (std::map<std::string, int>::iterator shapeIt = shapes.begin();
             shapeIt != shapes.end(); ++shapeIt)
        {

            Ogre::Node* bone;
            if (shapeIt->first.empty())
                // HACK: see NifSkeletonLoader::buildBones
                bone = animation->getNode(" ");
            else
                bone = animation->getNode(shapeIt->first);

            if (bone == NULL)
                throw std::runtime_error("can't find bone");

            btCompoundShape* compound = dynamic_cast<btCompoundShape*>(instance.mCompound);

            btTransform trans;
            trans.setOrigin(BtOgre::Convert::toBullet(bone->_getDerivedPosition()));
            trans.setRotation(BtOgre::Convert::toBullet(bone->_getDerivedOrientation()));

            compound->getChildShape(shapeIt->second)->setLocalScaling(BtOgre::Convert::toBullet(bone->_getDerivedScale()));
            compound->updateChildTransform(shapeIt->second, trans);
        }
    }
}

}


namespace MWWorld
{

    static const float sMaxSlope = 49.0f;
    static const float sStepSize = 32.0f;
    // Arbitrary number. To prevent infinite loops. They shouldn't happen but it's good to be prepared.
    static const int sMaxIterations = 8;

    class MovementSolver
    {
    private:
        static float getSlope(const Ogre::Vector3 &normal)
        {
            return normal.angleBetween(Ogre::Vector3(0.0f,0.0f,1.0f)).valueDegrees();
        }

        static bool stepMove(btCollisionObject *colobj, Ogre::Vector3 &position,
                             const Ogre::Vector3 &velocity, float &remainingTime,
                             OEngine::Physic::PhysicEngine *engine)
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
             *    - having moved forward by between epsilon() and velocity*remainingTime,
             *        = moved down between 0 and just under sStepSize but slope was too steep, or
             *        = moved the full sStepSize down (FIXME: this could be a bug)
             *
             *
             *
             * Starting position.  Obstacle or stairs with height upto sStepSize in front.
             *
             *     +--+                          +--+       |XX
             *     |  | -------> velocity        |  |    +--+XX
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
            OEngine::Physic::ActorTracer tracer, stepper;

            stepper.doTrace(colobj, position, position+Ogre::Vector3(0.0f,0.0f,sStepSize), engine);
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
             *          |XX|      the moved amount is velocity*remainingTime*tracer.mFraction
             *          +--+
             *    ==============================================
             */
            tracer.doTrace(colobj, stepper.mEndPos, stepper.mEndPos + velocity*remainingTime, engine);
            if(tracer.mFraction < std::numeric_limits<float>::epsilon())
                return false; // didn't even move the smallest representable amount

            /*
             * Try moving back down sStepSize using stepper.
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
            stepper.doTrace(colobj, tracer.mEndPos, tracer.mEndPos-Ogre::Vector3(0.0f,0.0f,sStepSize), engine);
            if(stepper.mFraction < 1.0f && getSlope(stepper.mPlaneNormal) <= sMaxSlope)
            {
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
        static inline Ogre::Vector3 project(const Ogre::Vector3 u, const Ogre::Vector3 &v)
        {
            return v * u.dotProduct(v);
        }

        ///Helper for computing the character sliding
        static inline Ogre::Vector3 slide(Ogre::Vector3 direction, const Ogre::Vector3 &planeNormal)
        {
            return direction - project(direction, planeNormal);
        }


    public:
        static Ogre::Vector3 traceDown(const MWWorld::Ptr &ptr, OEngine::Physic::PhysicEngine *engine, float maxHeight)
        {
            const ESM::Position &refpos = ptr.getRefData().getPosition();
            Ogre::Vector3 position(refpos.pos);

            OEngine::Physic::PhysicActor *physicActor = engine->getCharacter(ptr.getRefData().getHandle());
            if (!physicActor)
                return position;

            OEngine::Physic::ActorTracer tracer;
            tracer.findGround(physicActor, position, position-Ogre::Vector3(0,0,maxHeight), engine);
            if(tracer.mFraction >= 1.0f)
            {
                physicActor->setOnGround(false);
                return position;
            }

            physicActor->setOnGround(getSlope(tracer.mPlaneNormal) <= sMaxSlope);

            return tracer.mEndPos;
        }

        static Ogre::Vector3 move(const MWWorld::Ptr &ptr, const Ogre::Vector3 &movement, float time,
                                  bool isFlying, float waterlevel, float slowFall, OEngine::Physic::PhysicEngine *engine
                                  , std::map<std::string, std::string>& collisionTracker
                                  , std::map<std::string, std::string>& standingCollisionTracker)
        {
            const ESM::Position &refpos = ptr.getRefData().getPosition();
            Ogre::Vector3 position(refpos.pos);

            // Early-out for totally static creatures
            // (Not sure if gravity should still apply?)
            if (!ptr.getClass().canWalk(ptr) && !ptr.getClass().canFly(ptr) && !ptr.getClass().canSwim(ptr))
                return position;

            /* Anything to collide with? */
            OEngine::Physic::PhysicActor *physicActor = engine->getCharacter(ptr.getRefData().getHandle());
            if(!physicActor || !physicActor->getCollisionMode())
            {
                return position +  (Ogre::Quaternion(Ogre::Radian(refpos.rot[2]), Ogre::Vector3::NEGATIVE_UNIT_Z) *
                                    Ogre::Quaternion(Ogre::Radian(refpos.rot[0]), Ogre::Vector3::NEGATIVE_UNIT_X))
                                * movement * time;
            }

            btCollisionObject *colobj = physicActor->getCollisionBody();
            Ogre::Vector3 halfExtents = physicActor->getHalfExtents();
            position.z += halfExtents.z;

            waterlevel -= halfExtents.z * 0.5;
            /*
             * A 3/4 submerged example
             *
             *  +---+
             *  |   |
             *  |   |                     <- (original waterlevel)
             *  |   |
             *  |   |  <- position        <- waterlevel
             *  |   |
             *  |   |
             *  |   |
             *  +---+  <- (original position)
             */

            OEngine::Physic::ActorTracer tracer;
            bool wasOnGround = false;
            bool isOnGround = false;
            Ogre::Vector3 inertia(0.0f);
            Ogre::Vector3 velocity;

            if(position.z < waterlevel || isFlying) // under water by 3/4 or can fly
            {
                // TODO: Shouldn't water have higher drag in calculating velocity?
                velocity = (Ogre::Quaternion(Ogre::Radian(refpos.rot[2]), Ogre::Vector3::NEGATIVE_UNIT_Z)*
                            Ogre::Quaternion(Ogre::Radian(refpos.rot[0]), Ogre::Vector3::NEGATIVE_UNIT_X)) * movement;
            }
            else
            {
                velocity = Ogre::Quaternion(Ogre::Radian(refpos.rot[2]), Ogre::Vector3::NEGATIVE_UNIT_Z) * movement;
                // not in water nor can fly, so need to deal with gravity
                if(!physicActor->getOnGround()) // if current OnGround status is false, must be falling or jumping
                {
                    // If falling, add part of the incoming velocity with the current inertia
                    // TODO: but we could be jumping up?
                    velocity = velocity * time + physicActor->getInertialForce();

                    // avoid getting infinite inertia in air
                    float actorSpeed = ptr.getClass().getSpeed(ptr);
                    float speedXY = Ogre::Vector2(velocity.x, velocity.y).length();
                    if (speedXY > actorSpeed) 
                    {
                        velocity.x *= actorSpeed / speedXY;
                        velocity.y *= actorSpeed / speedXY;
                    }
                }
                inertia = velocity; // NOTE: velocity is for z axis only in this code block

                if(!(movement.z > 0.0f)) // falling or moving horizontally (or stationary?) check if we're on ground now
                {
                    wasOnGround = physicActor->getOnGround(); // store current state
                    tracer.doTrace(colobj, position, position - Ogre::Vector3(0,0,2), engine); // check if down 2 possible
                    if(tracer.mFraction < 1.0f && getSlope(tracer.mPlaneNormal) <= sMaxSlope)
                    {
                        const btCollisionObject* standingOn = tracer.mHitObject;
                        if (const OEngine::Physic::RigidBody* body = dynamic_cast<const OEngine::Physic::RigidBody*>(standingOn))
                        {
                            standingCollisionTracker[ptr.getRefData().getHandle()] = body->mName;
                        }
                        isOnGround = true;
                        // if we're on the ground, don't try to fall any more
                        velocity.z = std::max(0.0f, velocity.z);
                    }
                }
            }

            // Now that we have the effective movement vector, apply wind forces to it
            if (MWBase::Environment::get().getWorld()->isInStorm())
            {
                Ogre::Vector3 stormDirection = MWBase::Environment::get().getWorld()->getStormDirection();
                Ogre::Degree angle = stormDirection.angleBetween(velocity);
                static const float fStromWalkMult = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>()
                        .find("fStromWalkMult")->getFloat();
                velocity *= 1.f-(fStromWalkMult * (angle.valueDegrees()/180.f));
            }

            Ogre::Vector3 newPosition = position;
            /*
             * A loop to find newPosition using tracer, if successful different from the starting position.
             * nextpos is the local variable used to find potential newPosition, using velocity and remainingTime
             * The initial velocity was set earlier (see above).
             */
            float remainingTime = time;
            for(int iterations = 0; iterations < sMaxIterations && remainingTime > 0.01f; ++iterations)
            {
                // NOTE: velocity is either z axis only or x & z axis
                Ogre::Vector3 nextpos = newPosition + velocity * remainingTime;

                // If not able to fly, don't allow to swim up into the air
                // TODO: this if condition may not work for large creatures or situations
                //        where the creature gets above the waterline for some reason
                if(newPosition.z < waterlevel && // started 3/4 under water
                   !isFlying &&  // can't fly
                   nextpos.z > waterlevel &&     // but about to go above water
                   newPosition.z <= waterlevel)
                {
                    const Ogre::Vector3 down(0,0,-1);
                    Ogre::Real movelen = velocity.normalise();
                    Ogre::Vector3 reflectdir = velocity.reflect(down);
                    reflectdir.normalise();
                    velocity = slide(reflectdir, down)*movelen;
                    // NOTE: remainingTime is unchanged before the loop continues
                    continue; // velocity updated, calculate nextpos again
                }

                if(newPosition.squaredDistance(nextpos) > 0.0001)
                {
                    // trace to where character would go if there were no obstructions
                    tracer.doTrace(colobj, newPosition, nextpos, engine);

                    // check for obstructions
                    if(tracer.mFraction >= 1.0f)
                    {
                        newPosition = tracer.mEndPos; // ok to move, so set newPosition
                        remainingTime *= (1.0f-tracer.mFraction); // FIXME: remainingTime is no longer used so don't set it?
                        break;
                    }
                    else
                    {
                        const btCollisionObject* standingOn = tracer.mHitObject;
                        if (const OEngine::Physic::RigidBody* body = dynamic_cast<const OEngine::Physic::RigidBody*>(standingOn))
                        {
                            collisionTracker[ptr.getRefData().getHandle()] = body->mName;
                        }
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
                    remainingTime *= (1.0f-tracer.mFraction); // FIXME: remainingTime is no longer used so don't set it?
                    break;
                }


                Ogre::Vector3 oldPosition = newPosition;
                // We hit something. Try to step up onto it. (NOTE: stepMove does not allow stepping over)
                // NOTE: stepMove modifies newPosition if successful
                if(stepMove(colobj, newPosition, velocity, remainingTime, engine))
                {
                    // don't let pure water creatures move out of water after stepMove
                    if((ptr.getClass().canSwim(ptr) && !ptr.getClass().canWalk(ptr)) 
                            && newPosition.z > (waterlevel - halfExtents.z * 0.5))
                        newPosition = oldPosition;
                    else // Only on the ground if there's gravity
                        isOnGround = !(newPosition.z < waterlevel || isFlying);
                }
                else
                {
                    // Can't move this way, try to find another spot along the plane
                    Ogre::Real movelen = velocity.normalise();
                    Ogre::Vector3 reflectdir = velocity.reflect(tracer.mPlaneNormal);
                    reflectdir.normalise();
                    velocity = slide(reflectdir, tracer.mPlaneNormal)*movelen;

                    // Do not allow sliding upward if there is gravity. Stepping will have taken
                    // care of that.
                    if(!(newPosition.z < waterlevel || isFlying))
                        velocity.z = std::min(velocity.z, 0.0f);
                }
            }

            if(isOnGround || wasOnGround)
            {
                tracer.doTrace(colobj, newPosition, newPosition - Ogre::Vector3(0,0,sStepSize+2.0f), engine);
                if(tracer.mFraction < 1.0f && getSlope(tracer.mPlaneNormal) <= sMaxSlope)
                {
                    newPosition.z = tracer.mEndPos.z + 1.0f;
                    isOnGround = true;
                }
                else
                    isOnGround = false;
            }

            if(isOnGround || newPosition.z < waterlevel || isFlying)
                physicActor->setInertialForce(Ogre::Vector3(0.0f));
            else
            {
                float diff = time*-627.2f;
                if (inertia.z < 0)
                    diff *= slowFall;
                inertia.z += diff;
                physicActor->setInertialForce(inertia);
            }
            physicActor->setOnGround(isOnGround);

            newPosition.z -= halfExtents.z; // remove what was added at the beggining
            return newPosition;
        }
    };


    PhysicsSystem::PhysicsSystem(OEngine::Render::OgreRenderer &_rend) :
        mRender(_rend), mEngine(0), mTimeAccum(0.0f)
    {
        // Create physics. shapeLoader is deleted by the physic engine
        NifBullet::ManualBulletShapeLoader* shapeLoader = new NifBullet::ManualBulletShapeLoader();
        mEngine = new OEngine::Physic::PhysicEngine(shapeLoader);
    }

    PhysicsSystem::~PhysicsSystem()
    {
        delete mEngine;
    }

    OEngine::Physic::PhysicEngine* PhysicsSystem::getEngine()
    {
        return mEngine;
    }

    std::pair<float, std::string> PhysicsSystem::getFacedHandle(float queryDistance)
    {
        Ray ray = mRender.getCamera()->getCameraToViewportRay(0.5, 0.5);

        Ogre::Vector3 origin_ = ray.getOrigin();
        btVector3 origin(origin_.x, origin_.y, origin_.z);
        Ogre::Vector3 dir_ = ray.getDirection().normalisedCopy();
        btVector3 dir(dir_.x, dir_.y, dir_.z);

        btVector3 dest = origin + dir * queryDistance;
        std::pair <std::string, float> result = mEngine->rayTest(origin, dest);
        result.second *= queryDistance;

        return std::make_pair (result.second, result.first);
    }

    std::vector < std::pair <float, std::string> > PhysicsSystem::getFacedHandles (float queryDistance)
    {
        Ray ray = mRender.getCamera()->getCameraToViewportRay(0.5, 0.5);

        Ogre::Vector3 origin_ = ray.getOrigin();
        btVector3 origin(origin_.x, origin_.y, origin_.z);
        Ogre::Vector3 dir_ = ray.getDirection().normalisedCopy();
        btVector3 dir(dir_.x, dir_.y, dir_.z);

        btVector3 dest = origin + dir * queryDistance;
        std::vector < std::pair <float, std::string> > results;
        /* auto */ results = mEngine->rayTest2(origin, dest);
        std::vector < std::pair <float, std::string> >::iterator i;
        for (/* auto */ i = results.begin (); i != results.end (); ++i)
            i->first *= queryDistance;
        return results;
    }

    std::vector < std::pair <float, std::string> > PhysicsSystem::getFacedHandles (float mouseX, float mouseY, float queryDistance)
    {
        Ray ray = mRender.getCamera()->getCameraToViewportRay(mouseX, mouseY);
        Ogre::Vector3 from = ray.getOrigin();
        Ogre::Vector3 to = ray.getPoint(queryDistance);

        btVector3 _from, _to;
        _from = btVector3(from.x, from.y, from.z);
        _to = btVector3(to.x, to.y, to.z);

        std::vector < std::pair <float, std::string> > results;
        /* auto */ results = mEngine->rayTest2(_from,_to);
        std::vector < std::pair <float, std::string> >::iterator i;
        for (/* auto */ i = results.begin (); i != results.end (); ++i)
            i->first *= queryDistance;
        return results;
    }

    std::pair<std::string,Ogre::Vector3> PhysicsSystem::getHitContact(const std::string &name,
                                                                      const Ogre::Vector3 &origin,
                                                                      const Ogre::Quaternion &orient,
                                                                      float queryDistance)
    {
        const MWWorld::Store<ESM::GameSetting> &store = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

        btConeShape shape(Ogre::Degree(store.find("fCombatAngleXY")->getFloat()/2.0f).valueRadians(),
                          queryDistance);
        shape.setLocalScaling(btVector3(1, 1, Ogre::Degree(store.find("fCombatAngleZ")->getFloat()/2.0f).valueRadians() /
                                              shape.getRadius()));

        // The shape origin is its center, so we have to move it forward by half the length. The
        // real origin will be provided to getFilteredContact to find the closest.
        Ogre::Vector3 center = origin + (orient * Ogre::Vector3(0.0f, queryDistance*0.5f, 0.0f));

        btCollisionObject object;
        object.setCollisionShape(&shape);
        object.setWorldTransform(btTransform(btQuaternion(orient.x, orient.y, orient.z, orient.w),
                                             btVector3(center.x, center.y, center.z)));

        std::pair<const OEngine::Physic::RigidBody*,btVector3> result = mEngine->getFilteredContact(
                name, btVector3(origin.x, origin.y, origin.z), &object);
        if(!result.first)
            return std::make_pair(std::string(), Ogre::Vector3(&result.second[0]));
        return std::make_pair(result.first->mName, Ogre::Vector3(&result.second[0]));
    }


    bool PhysicsSystem::castRay(const Vector3& from, const Vector3& to, bool raycastingObjectOnly,bool ignoreHeightMap)
    {
        btVector3 _from, _to;
        _from = btVector3(from.x, from.y, from.z);
        _to = btVector3(to.x, to.y, to.z);

        std::pair<std::string, float> result = mEngine->rayTest(_from, _to, raycastingObjectOnly,ignoreHeightMap);
        return !(result.first == "");
    }

    std::pair<bool, Ogre::Vector3>
    PhysicsSystem::castRay(const Ogre::Vector3 &orig, const Ogre::Vector3 &dir, float len)
    {
        Ogre::Ray ray = Ogre::Ray(orig, dir);
        Ogre::Vector3 to = ray.getPoint(len);

        btVector3 btFrom = btVector3(orig.x, orig.y, orig.z);
        btVector3 btTo = btVector3(to.x, to.y, to.z);

        std::pair<std::string, float> test = mEngine->rayTest(btFrom, btTo);
        if (test.second == -1) {
            return std::make_pair(false, Ogre::Vector3());
        }
        return std::make_pair(true, ray.getPoint(len * test.second));
    }

    std::pair<bool, Ogre::Vector3> PhysicsSystem::castRay(float mouseX, float mouseY, Ogre::Vector3* normal, std::string* hit)
    {
        Ogre::Ray ray = mRender.getCamera()->getCameraToViewportRay(
            mouseX,
            mouseY);
        Ogre::Vector3 from = ray.getOrigin();
        Ogre::Vector3 to = ray.getPoint(200); /// \todo make this distance (ray length) configurable

        btVector3 _from, _to;
        _from = btVector3(from.x, from.y, from.z);
        _to = btVector3(to.x, to.y, to.z);

        std::pair<std::string, float> result = mEngine->rayTest(_from, _to, true, false, normal);

        if (result.first == "")
            return std::make_pair(false, Ogre::Vector3());
        else
        {
            if (hit != NULL)
                *hit = result.first;
            return std::make_pair(true, ray.getPoint(200*result.second));  /// \todo make this distance (ray length) configurable
        }
    }

    std::vector<std::string> PhysicsSystem::getCollisions(const Ptr &ptr)
    {
        return mEngine->getCollisions(ptr.getRefData().getBaseNode()->getName());
    }

    Ogre::Vector3 PhysicsSystem::traceDown(const MWWorld::Ptr &ptr, float maxHeight)
    {
        return MovementSolver::traceDown(ptr, mEngine, maxHeight);
    }

    void PhysicsSystem::addHeightField (float* heights,
                int x, int y, float yoffset,
                float triSize, float sqrtVerts)
    {
        mEngine->addHeightField(heights, x, y, yoffset, triSize, sqrtVerts);
    }

    void PhysicsSystem::removeHeightField (int x, int y)
    {
        mEngine->removeHeightField(x, y);
    }

    void PhysicsSystem::addObject (const Ptr& ptr, bool placeable)
    {
        std::string mesh = ptr.getClass().getModel(ptr);
        Ogre::SceneNode* node = ptr.getRefData().getBaseNode();
        handleToMesh[node->getName()] = mesh;
        mEngine->createAndAdjustRigidBody(
            mesh, node->getName(), node->getScale().x, node->getPosition(), node->getOrientation(), 0, 0, false, placeable);
        mEngine->createAndAdjustRigidBody(
            mesh, node->getName(), node->getScale().x, node->getPosition(), node->getOrientation(), 0, 0, true, placeable);
    }

    void PhysicsSystem::addActor (const Ptr& ptr)
    {
        std::string mesh = ptr.getClass().getModel(ptr);
        Ogre::SceneNode* node = ptr.getRefData().getBaseNode();
        //TODO:optimize this. Searching the std::map isn't very efficient i think.
        mEngine->addCharacter(node->getName(), mesh, node->getPosition(), node->getScale().x, node->getOrientation());
    }

    void PhysicsSystem::removeObject (const std::string& handle)
    {
        mEngine->removeCharacter(handle);
        mEngine->removeRigidBody(handle);
        mEngine->deleteRigidBody(handle);
    }

    void PhysicsSystem::moveObject (const Ptr& ptr)
    {
        Ogre::SceneNode *node = ptr.getRefData().getBaseNode();
        const std::string &handle = node->getName();
        const Ogre::Vector3 &position = node->getPosition();

        if(OEngine::Physic::RigidBody *body = mEngine->getRigidBody(handle))
            body->getWorldTransform().setOrigin(btVector3(position.x,position.y,position.z));

        if(OEngine::Physic::RigidBody *body = mEngine->getRigidBody(handle, true))
            body->getWorldTransform().setOrigin(btVector3(position.x,position.y,position.z));

        if(OEngine::Physic::PhysicActor *physact = mEngine->getCharacter(handle))
            physact->setPosition(position);
    }

    void PhysicsSystem::rotateObject (const Ptr& ptr)
    {
        Ogre::SceneNode* node = ptr.getRefData().getBaseNode();
        const std::string &handle = node->getName();
        const Ogre::Quaternion &rotation = node->getOrientation();

        // TODO: map to MWWorld::Ptr for faster access
        if (OEngine::Physic::PhysicActor* act = mEngine->getCharacter(handle))
        {
            act->setRotation(rotation);
        }
        if (OEngine::Physic::RigidBody* body = mEngine->getRigidBody(handle))
        {
            if(dynamic_cast<btBoxShape*>(body->getCollisionShape()) == NULL)
                body->getWorldTransform().setRotation(btQuaternion(rotation.x, rotation.y, rotation.z, rotation.w));
            else
                mEngine->boxAdjustExternal(handleToMesh[handle], body, node->getScale().x, node->getPosition(), rotation);
        }
        if (OEngine::Physic::RigidBody* body = mEngine->getRigidBody(handle, true))
        {
            if(dynamic_cast<btBoxShape*>(body->getCollisionShape()) == NULL)
                body->getWorldTransform().setRotation(btQuaternion(rotation.x, rotation.y, rotation.z, rotation.w));
            else
                mEngine->boxAdjustExternal(handleToMesh[handle], body, node->getScale().x, node->getPosition(), rotation);
        }
    }

    void PhysicsSystem::scaleObject (const Ptr& ptr)
    {
        Ogre::SceneNode* node = ptr.getRefData().getBaseNode();
        const std::string &handle = node->getName();
        if(handleToMesh.find(handle) != handleToMesh.end())
        {
            bool placeable = false;
            if (OEngine::Physic::RigidBody* body = mEngine->getRigidBody(handle,true))
                placeable = body->mPlaceable;
            else if (OEngine::Physic::RigidBody* body = mEngine->getRigidBody(handle,false))
                placeable = body->mPlaceable;
            removeObject(handle);
            addObject(ptr, placeable);
        }

        if (OEngine::Physic::PhysicActor* act = mEngine->getCharacter(handle))
            act->setScale(node->getScale().x);
    }

    bool PhysicsSystem::toggleCollisionMode()
    {
        for(std::map<std::string,OEngine::Physic::PhysicActor*>::iterator it = mEngine->mActorMap.begin(); it != mEngine->mActorMap.end();++it)
        {
            if (it->first=="player")
            {
                OEngine::Physic::PhysicActor* act = it->second;

                bool cmode = act->getCollisionMode();
                if(cmode)
                {
                    act->enableCollisionMode(false);
                    return false;
                }
                else
                {
                    act->enableCollisionMode(true);
                    return true;
                }
            }
        }

        throw std::logic_error ("can't find player");
    }

    bool PhysicsSystem::getObjectAABB(const MWWorld::Ptr &ptr, Ogre::Vector3 &min, Ogre::Vector3 &max)
    {
        std::string model = ptr.getClass().getModel(ptr);
        if (model.empty()) {
            return false;
        }
        btVector3 btMin, btMax;
        float scale = ptr.getCellRef().getScale();
        mEngine->getObjectAABB(model, scale, btMin, btMax);

        min.x = btMin.x();
        min.y = btMin.y();
        min.z = btMin.z();

        max.x = btMax.x();
        max.y = btMax.y();
        max.z = btMax.z();

        return true;
    }


    void PhysicsSystem::queueObjectMovement(const Ptr &ptr, const Ogre::Vector3 &movement)
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

    const PtrVelocityList& PhysicsSystem::applyQueuedMovement(float dt)
    {
        // Collision events are only tracked for a single frame, so reset first
        mCollisions.clear();
        mStandingCollisions.clear();

        mMovementResults.clear();

        mTimeAccum += dt;
        if(mTimeAccum >= 1.0f/60.0f)
        {
            const MWBase::World *world = MWBase::Environment::get().getWorld();
            PtrVelocityList::iterator iter = mMovementQueue.begin();
            for(;iter != mMovementQueue.end();++iter)
            {
                float waterlevel = -std::numeric_limits<float>::max();
                const ESM::Cell *cell = iter->first.getCell()->getCell();
                if(cell->hasWater())
                    waterlevel = cell->mWater;

                float oldHeight = iter->first.getRefData().getPosition().pos[2];

                const MWMechanics::MagicEffects& effects = iter->first.getClass().getCreatureStats(iter->first).getMagicEffects();

                bool waterCollision = false;
                if (effects.get(ESM::MagicEffect::WaterWalking).mMagnitude
                        && cell->hasWater()
                        && !world->isUnderwater(iter->first.getCell(),
                                               Ogre::Vector3(iter->first.getRefData().getPosition().pos)))
                    waterCollision = true;

                btStaticPlaneShape planeShape(btVector3(0,0,1), waterlevel);
                btCollisionObject object;
                object.setCollisionShape(&planeShape);

                // TODO: this seems to have a slight performance impact
                if (waterCollision)
                    mEngine->mDynamicsWorld->addCollisionObject(&object,
                                                                0xff, OEngine::Physic::CollisionType_Actor);

                // 100 points of slowfall reduce gravity by 90% (this is just a guess)
                float slowFall = 1-std::min(std::max(0.f, (effects.get(ESM::MagicEffect::SlowFall).mMagnitude / 100.f) * 0.9f), 0.9f);

                Ogre::Vector3 newpos = MovementSolver::move(iter->first, iter->second, mTimeAccum,
                                                            world->isFlying(iter->first),
                                                            waterlevel, slowFall, mEngine, mCollisions, mStandingCollisions);

                if (waterCollision)
                    mEngine->mDynamicsWorld->removeCollisionObject(&object);

                float heightDiff = newpos.z - oldHeight;

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
        animateCollisionShapes(mEngine->mAnimatedShapes);
        animateCollisionShapes(mEngine->mAnimatedRaycastingShapes);

        mEngine->stepSimulation(dt);
    }

    bool PhysicsSystem::isActorStandingOn(const Ptr &actor, const Ptr &object) const
    {
        const std::string& actorHandle = actor.getRefData().getHandle();
        const std::string& objectHandle = object.getRefData().getHandle();

        for (std::map<std::string, std::string>::const_iterator it = mStandingCollisions.begin();
             it != mStandingCollisions.end(); ++it)
        {
            if (it->first == actorHandle && it->second == objectHandle)
                return true;
        }
        return false;
    }

    void PhysicsSystem::getActorsStandingOn(const Ptr &object, std::vector<std::string> &out) const
    {
        const std::string& objectHandle = object.getRefData().getHandle();

        for (std::map<std::string, std::string>::const_iterator it = mStandingCollisions.begin();
             it != mStandingCollisions.end(); ++it)
        {
            if (it->second == objectHandle)
                out.push_back(it->first);
        }
    }

    bool PhysicsSystem::isActorCollidingWith(const Ptr &actor, const Ptr &object) const
    {
        const std::string& actorHandle = actor.getRefData().getHandle();
        const std::string& objectHandle = object.getRefData().getHandle();

        for (std::map<std::string, std::string>::const_iterator it = mCollisions.begin();
             it != mCollisions.end(); ++it)
        {
            if (it->first == actorHandle && it->second == objectHandle)
                return true;
        }
        return false;
    }

    void PhysicsSystem::getActorsCollidingWith(const Ptr &object, std::vector<std::string> &out) const
    {
        const std::string& objectHandle = object.getRefData().getHandle();

        for (std::map<std::string, std::string>::const_iterator it = mCollisions.begin();
             it != mCollisions.end(); ++it)
        {
            if (it->second == objectHandle)
                out.push_back(it->first);
        }
    }

}
