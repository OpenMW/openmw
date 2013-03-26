#include "physicssystem.hpp"

#include <stdexcept>

#include <OgreRoot.h>
#include <OgreRenderWindow.h>
#include <OgreSceneManager.h>
#include <OgreViewport.h>
#include <OgreCamera.h>
#include <OgreTextureManager.h>

#include <openengine/bullet/trace.h>
#include <openengine/bullet/physic.hpp>
#include <openengine/ogre/renderer.hpp>

#include <components/nifbullet/bulletnifloader.hpp>

//#include "../mwbase/world.hpp" // FIXME
#include "../mwbase/environment.hpp"

#include "ptr.hpp"
#include "class.hpp"

using namespace Ogre;
namespace MWWorld
{

    static const float sMaxSlope = 60.0f;
    static const float sStepSize = 30.0f;
    // Arbitrary number. To prevent infinite loops. They shouldn't happen but it's good to be prepared.
    static const int sMaxIterations = 50;

    class MovementSolver
    {
    private:
        static bool stepMove(Ogre::Vector3& position, const Ogre::Vector3 &velocity, float remainingTime,
                             const Ogre::Vector3 &halfExtents, bool isInterior,
                             OEngine::Physic::PhysicEngine *engine)
        {
            traceResults trace; // no initialization needed

            newtrace(&trace, position, position+Ogre::Vector3(0.0f,0.0f,sStepSize),
                     halfExtents, isInterior, engine);
            if(trace.fraction == 0.0f)
                return false;

            newtrace(&trace, trace.endpos, trace.endpos + velocity*remainingTime,
                     halfExtents, isInterior, engine);
            if(trace.fraction == 0.0f || (trace.fraction != 1.0f && getSlope(trace.planenormal) > sMaxSlope))
                return false;

            newtrace(&trace, trace.endpos, trace.endpos-Ogre::Vector3(0.0f,0.0f,sStepSize), halfExtents, isInterior, engine);
            if(getSlope(trace.planenormal) <= sMaxSlope)
            {
                // only step down onto semi-horizontal surfaces. don't step down onto the side of a house or a wall.
                position = trace.endpos;
                return true;
            }

            return false;
        }

        static void clipVelocity(Ogre::Vector3& inout, const Ogre::Vector3& normal, float overbounce=1.0f)
        {
            //Math stuff. Basically just project the velocity vector onto the plane represented by the normal.
            //More specifically, it projects velocity onto the normal, takes that result, multiplies it by overbounce and then subtracts it from velocity.
            float backoff = inout.dotProduct(normal);
            if(backoff < 0.0f)
                backoff *= overbounce;
            else
                backoff /= overbounce;

            inout -= normal*backoff;
        }

        static void projectVelocity(Ogre::Vector3& velocity, const Ogre::Vector3& direction)
        {
            Ogre::Vector3 normalizedDirection(direction);
            normalizedDirection.normalise();

            // no divide by normalizedDirection.length necessary because it's normalized
            velocity = normalizedDirection * velocity.dotProduct(normalizedDirection);
        }

        static float getSlope(const Ogre::Vector3 &normal)
        {
            return normal.angleBetween(Ogre::Vector3(0.0f,0.0f,1.0f)).valueDegrees();
        }

    public:
        static Ogre::Vector3 move(const MWWorld::Ptr &ptr, const Ogre::Vector3 &movement, float time,
                                  bool gravity, OEngine::Physic::PhysicEngine *engine)
        {
            const ESM::Position &refpos = ptr.getRefData().getPosition();
            Ogre::Vector3 position(refpos.pos);

            /* Anything to collide with? */
            OEngine::Physic::PhysicActor *physicActor = engine->getCharacter(ptr.getRefData().getHandle());
            if(!physicActor || !physicActor->getCollisionMode())
            {
                // FIXME: This works, but it's inconcsistent with how the rotations are applied elsewhere. Why?
                return position + (Ogre::Quaternion(Ogre::Radian( -refpos.rot[2]), Ogre::Vector3::UNIT_Z)*
                                   Ogre::Quaternion(Ogre::Radian( -refpos.rot[1]), Ogre::Vector3::UNIT_Y)*
                                   Ogre::Quaternion(Ogre::Radian( -refpos.rot[0]), Ogre::Vector3::UNIT_X)) *
                                  movement;
            }

            traceResults trace; //no initialization needed
            bool onground = false;
            float remainingTime = time;
            bool isInterior = !ptr.getCell()->isExterior();
            Ogre::Vector3 halfExtents = physicActor->getHalfExtents();// + Vector3(1,1,1);
            physicActor->enableCollisions(false);

            Ogre::Vector3 velocity;
            if(!gravity)
            {
                velocity = (Ogre::Quaternion(Ogre::Radian( -refpos.rot[2]), Ogre::Vector3::UNIT_Z)*
                            Ogre::Quaternion(Ogre::Radian( -refpos.rot[1]), Ogre::Vector3::UNIT_Y)*
                            Ogre::Quaternion(Ogre::Radian( -refpos.rot[0]), Ogre::Vector3::UNIT_X)) *
                           movement / time;
            }
            else
            {
                if(!(movement.z > 0.0f))
                {
                    newtrace(&trace, position, position-Ogre::Vector3(0,0,4), halfExtents, isInterior, engine);
                    if(trace.fraction < 1.0f && getSlope(trace.planenormal) <= sMaxSlope)
                        onground = true;
                }
                velocity = Ogre::Quaternion(Ogre::Radian(-refpos.rot[2]), Ogre::Vector3::UNIT_Z)*movement / time;
                velocity.z += physicActor->getVerticalForce();
            }

            Ogre::Vector3 clippedVelocity(velocity);
            if(onground)
            {
                // if we're on the ground, force velocity to track it
                clippedVelocity.z = velocity.z = std::max(0.0f, velocity.z);
                clipVelocity(clippedVelocity, trace.planenormal);
            }

            const Ogre::Vector3 up(0.0f, 0.0f, 1.0f);
            Ogre::Vector3 newPosition = position;
            int iterations = 0;
            do {
                // trace to where character would go if there were no obstructions
                newtrace(&trace, newPosition, newPosition+clippedVelocity*remainingTime, halfExtents, isInterior, engine);
                newPosition = trace.endpos;
                remainingTime = remainingTime * (1.0f-trace.fraction);

                // check for obstructions
                if(trace.fraction < 1.0f)
                {
                    //std::cout<<"angle: "<<getSlope(trace.planenormal)<<"\n";
                    if(getSlope(trace.planenormal) <= sMaxSlope)
                    {
                        // We hit a slope we can walk on. Update velocity accordingly.
                        clipVelocity(clippedVelocity, trace.planenormal);
                        // We're only on the ground if gravity is affecting us
                        onground = gravity;
                    }
                    else
                    {
                        // Can't walk on this. Try to step up onto it.
                        if((gravity && !onground) ||
                           !stepMove(newPosition, velocity, remainingTime, halfExtents, isInterior, engine))
                        {
                            Ogre::Vector3 resultantDirection = trace.planenormal.crossProduct(up);
                            resultantDirection.normalise();
                            clippedVelocity = velocity;
                            projectVelocity(clippedVelocity, resultantDirection);

                            // just this isn't enough sometimes. It's the same problem that causes steps to be necessary on even uphill terrain.
                            clippedVelocity += trace.planenormal*clippedVelocity.length()/50.0f;
                        }
                    }
                }

                iterations++;
            } while(iterations < sMaxIterations && remainingTime > 0.0f);

            if(onground)
            {
                newtrace(&trace, newPosition, newPosition-Ogre::Vector3(0,0,sStepSize+4.0f), halfExtents, isInterior, engine);
                if(trace.fraction < 1.0f && getSlope(trace.planenormal) <= sMaxSlope)
                    newPosition.z = trace.endpos.z + 2.0f;
                else
                    onground = false;
            }
            physicActor->setOnGround(onground);
            physicActor->setVerticalForce(!onground ? clippedVelocity.z - time*627.2f : 0.0f);
            physicActor->enableCollisions(true);
            return newPosition;
        }
    };


    PhysicsSystem::PhysicsSystem(OEngine::Render::OgreRenderer &_rend) :
        mRender(_rend), mEngine(0)
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

    std::pair<float, std::string> PhysicsSystem::getFacedHandle (MWWorld::World& world, float queryDistance)
    {
        btVector3 dir(0, 1, 0);
        dir = dir.rotate(btVector3(1, 0, 0), mPlayerData.pitch);
        dir = dir.rotate(btVector3(0, 0, 1), mPlayerData.yaw);
        dir.setX(-dir.x());

        btVector3 origin(
            mPlayerData.eyepos.x,
            mPlayerData.eyepos.y,
            mPlayerData.eyepos.z);
        origin += dir * 5;

        btVector3 dest = origin + dir * queryDistance;
        std::pair <std::string, float> result;
        /*auto*/ result = mEngine->rayTest(origin, dest);
        result.second *= queryDistance;
        return std::make_pair (result.second, result.first);
    }

    std::vector < std::pair <float, std::string> > PhysicsSystem::getFacedHandles (float queryDistance)
    {
        btVector3 dir(0, 1, 0);
        dir = dir.rotate(btVector3(1, 0, 0), mPlayerData.pitch);
        dir = dir.rotate(btVector3(0, 0, 1), mPlayerData.yaw);
        dir.setX(-dir.x());

        btVector3 origin(
            mPlayerData.eyepos.x,
            mPlayerData.eyepos.y,
            mPlayerData.eyepos.z);
        origin += dir * 5;

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

    void PhysicsSystem::setCurrentWater(bool hasWater, int waterHeight)
    {
        // TODO: store and use
    }

    btVector3 PhysicsSystem::getRayPoint(float extent)
    {
        //get a ray pointing to the center of the viewport
        Ray centerRay = mRender.getCamera()->getCameraToViewportRay(
        mRender.getViewport()->getWidth()/2,
        mRender.getViewport()->getHeight()/2);
        btVector3 result(centerRay.getPoint(extent).x,centerRay.getPoint(extent).y,centerRay.getPoint(extent).z);
        return result;
    }

    btVector3 PhysicsSystem::getRayPoint(float extent, float mouseX, float mouseY)
    {
        //get a ray pointing to the center of the viewport
        Ray centerRay = mRender.getCamera()->getCameraToViewportRay(mouseX, mouseY);
        btVector3 result(centerRay.getPoint(extent).x,centerRay.getPoint(extent).y,centerRay.getPoint(extent).z);
        return result;
    }

    bool PhysicsSystem::castRay(const Vector3& from, const Vector3& to)
    {
        btVector3 _from, _to;
        _from = btVector3(from.x, from.y, from.z);
        _to = btVector3(to.x, to.y, to.z);

        std::pair<std::string, float> result = mEngine->rayTest(_from, _to);

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
        if (test.first == "") {
            return std::make_pair(false, Ogre::Vector3());
        }
        return std::make_pair(true, ray.getPoint(len * test.second));
    }

    std::pair<bool, Ogre::Vector3> PhysicsSystem::castRay(float mouseX, float mouseY)
    {
        Ogre::Ray ray = mRender.getCamera()->getCameraToViewportRay(
            mouseX,
            mouseY);
        Ogre::Vector3 from = ray.getOrigin();
        Ogre::Vector3 to = ray.getPoint(200); /// \todo make this distance (ray length) configurable

        btVector3 _from, _to;
        _from = btVector3(from.x, from.y, from.z);
        _to = btVector3(to.x, to.y, to.z);

        std::pair<std::string, float> result = mEngine->rayTest(_from, _to);

        if (result.first == "")
            return std::make_pair(false, Ogre::Vector3());
        else
        {
            return std::make_pair(true, ray.getPoint(200*result.second));  /// \todo make this distance (ray length) configurable
        }
    }

    Ogre::Vector3 PhysicsSystem::move(const MWWorld::Ptr &ptr, const Ogre::Vector3 &movement, float time, bool gravity)
    {
        return MovementSolver::move(ptr, movement, time, gravity, mEngine);
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
        std::string mesh = MWWorld::Class::get(ptr).getModel(ptr);
        Ogre::SceneNode* node = ptr.getRefData().getBaseNode();
        handleToMesh[node->getName()] = mesh;
        OEngine::Physic::RigidBody* body = mEngine->createAndAdjustRigidBody(
            mesh, node->getName(), node->getScale().x, node->getPosition(), node->getOrientation(), 0, 0, false, placeable);
        OEngine::Physic::RigidBody* raycastingBody = mEngine->createAndAdjustRigidBody(
            mesh, node->getName(), node->getScale().x, node->getPosition(), node->getOrientation(), 0, 0, true, placeable);
        mEngine->addRigidBody(body, true, raycastingBody);
    }

    void PhysicsSystem::addActor (const Ptr& ptr)
    {
        std::string mesh = MWWorld::Class::get(ptr).getModel(ptr);
        Ogre::SceneNode* node = ptr.getRefData().getBaseNode();
        //TODO:optimize this. Searching the std::map isn't very efficient i think.
        mEngine->addCharacter(node->getName(), mesh, node->getPosition(), node->getScale().x, node->getOrientation());
    }

    void PhysicsSystem::removeObject (const std::string& handle)
    {
        //TODO:check if actor???

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
        if (OEngine::Physic::PhysicActor* act = mEngine->getCharacter(handle))
        {
            //Needs to be changed
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
        for(std::map<std::string,OEngine::Physic::PhysicActor*>::iterator it = mEngine->PhysicActorMap.begin(); it != mEngine->PhysicActorMap.end();it++)
        {
            if (it->first=="player")
            {
                OEngine::Physic::PhysicActor* act = it->second;

                bool cmode = act->getCollisionMode();
                if(cmode)
                {
                    act->enableCollisions(false);
                    return false;
                }
                else
                {
                    act->enableCollisions(true);
                    return true;
                }
            }
        }

        throw std::logic_error ("can't find player");
    }

    bool PhysicsSystem::getObjectAABB(const MWWorld::Ptr &ptr, Ogre::Vector3 &min, Ogre::Vector3 &max)
    {
        std::string model = MWWorld::Class::get(ptr).getModel(ptr);
        if (model.empty()) {
            return false;
        }
        btVector3 btMin, btMax;
        float scale = ptr.getCellRef().mScale;
        mEngine->getObjectAABB(model, scale, btMin, btMax);

        min.x = btMin.x();
        min.y = btMin.y();
        min.z = btMin.z();

        max.x = btMax.x();
        max.y = btMax.y();
        max.z = btMax.z();

        return true;
    }

    void PhysicsSystem::updatePlayerData(Ogre::Vector3 &eyepos, float pitch, float yaw)
    {
        mPlayerData.eyepos = eyepos;
        mPlayerData.pitch = pitch;
        mPlayerData.yaw = yaw;
    }
}
