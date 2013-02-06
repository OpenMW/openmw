#include "physicssystem.hpp"

#include <stdexcept>

#include <OgreRoot.h>
#include <OgreRenderWindow.h>
#include <OgreSceneManager.h>
#include <OgreViewport.h>
#include <OgreCamera.h>
#include <OgreTextureManager.h>

#include <components/nifbullet/bullet_nif_loader.hpp>

//#include "../mwbase/world.hpp" // FIXME
#include "../mwbase/environment.hpp"

#include "ptr.hpp"
#include "class.hpp"

using namespace Ogre;
namespace MWWorld
{

    static const float sMaxSlope = 60.0f;

    class MovementSolver
    {
    private:
        static bool stepMove(Ogre::Vector3& position, const Ogre::Vector3 &velocity, float remainingTime,
                             float verticalRotation, const Ogre::Vector3 &halfExtents, bool isInterior,
                             OEngine::Physic::PhysicEngine *engine)
        {
            traceResults trace; // no initialization needed

            newtrace(&trace, position+Ogre::Vector3(0.0f,0.0f,STEPSIZE),
                             position+Ogre::Vector3(0.0f,0.0f,STEPSIZE)+velocity*remainingTime,
                    halfExtents, verticalRotation, isInterior, engine);
            if(trace.fraction == 0.0f || (trace.fraction != 1.0f && getSlope(trace.planenormal) > sMaxSlope))
                return false;

            newtrace(&trace, trace.endpos, trace.endpos-Ogre::Vector3(0,0,STEPSIZE), halfExtents, verticalRotation, isInterior, engine);
            if(getSlope(trace.planenormal) < sMaxSlope)
            {
                // only step down onto semi-horizontal surfaces. don't step down onto the side of a house or a wall.
                position = trace.endpos;
                return true;
            }

            return false;
        }

        static void clipVelocity(const Ogre::Vector3& in, const Ogre::Vector3& normal, Ogre::Vector3& out,
                                 const float overbounce)
        {
            //Math stuff. Basically just project the velocity vector onto the plane represented by the normal.
            //More specifically, it projects velocity onto the normal, takes that result, multiplies it by overbounce and then subtracts it from velocity.
            float backoff;

            backoff = in.dotProduct(normal);
            if(backoff < 0.0f)
                backoff *= overbounce;
            else
                backoff /= overbounce;

            out = in - (normal*backoff);
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
        static Ogre::Vector3 move(const MWWorld::Ptr &ptr, Ogre::Vector3 movement, float time,
                                  bool gravity, OEngine::Physic::PhysicEngine *engine)
        {
            const ESM::Position &refpos = ptr.getRefData().getPosition();
            Ogre::Vector3 position(refpos.pos);

            // Rotates first around z, then y, then x
            movement = (Ogre::Quaternion(Ogre::Radian(-refpos.rot[0]), Ogre::Vector3::UNIT_X)*
                        Ogre::Quaternion(Ogre::Radian(-refpos.rot[1]), Ogre::Vector3::UNIT_Y)*
                        Ogre::Quaternion(Ogre::Radian(-refpos.rot[2]), Ogre::Vector3::UNIT_Z)) *
                       movement;

            /* Anything to collide with? */
            OEngine::Physic::PhysicActor *physicActor = engine->getCharacter(ptr.getRefData().getHandle());
            if(!physicActor || !physicActor->getCollisionMode())
                return position + movement;

            traceResults trace; //no initialization needed
            int iterations=0, maxIterations=50; //arbitrary number. To prevent infinite loops. They shouldn't happen but it's good to be prepared.

            Ogre::Vector3 horizontalVelocity = movement/time;
            float verticalVelocity = (gravity ? physicActor->getVerticalForce() :
                                                horizontalVelocity.z);
            Ogre::Vector3 velocity(horizontalVelocity.x, horizontalVelocity.y, verticalVelocity); // we need a copy of the velocity before we start clipping it for steps
            Ogre::Vector3 clippedVelocity(horizontalVelocity.x, horizontalVelocity.y, verticalVelocity);

            float remainingTime = time;
            bool isInterior = !ptr.getCell()->isExterior();
            float verticalRotation = physicActor->getRotation().getYaw().valueDegrees();
            Ogre::Vector3 halfExtents = physicActor->getHalfExtents();

            Ogre::Vector3 lastNormal(0.0f);
            Ogre::Vector3 currentNormal(0.0f);
            Ogre::Vector3 up(0.0f, 0.0f, 1.0f);
            Ogre::Vector3 newPosition = position;

            if(gravity)
            {
                newtrace(&trace, position, position+Ogre::Vector3(0,0,-10), halfExtents, verticalRotation, isInterior, engine);
                if(trace.fraction < 1.0f)
                {
                    if(getSlope(trace.planenormal) > sMaxSlope)
                    {
                        // if we're on a really steep slope, don't listen to user input
                        clippedVelocity.x = clippedVelocity.y = 0.0f;
                    }
                    else
                    {
                        // if we're within 10 units of the ground, force velocity to track the ground
                        clipVelocity(clippedVelocity, trace.planenormal, clippedVelocity, 1.0f);
                    }
                }
            }

            do {
                // trace to where character would go if there were no obstructions
                newtrace(&trace, newPosition, newPosition+clippedVelocity*remainingTime, halfExtents, verticalRotation, isInterior, engine);
                newPosition = trace.endpos;
                currentNormal = trace.planenormal;
                remainingTime = remainingTime * (1.0f-trace.fraction);

                // check for obstructions
                if(trace.fraction != 1.0f)
                {
                    //std::cout<<"angle: "<<getSlope(trace.planenormal)<<"\n";
                    if(getSlope(currentNormal) > sMaxSlope || currentNormal == lastNormal)
                    {
                        if(!stepMove(newPosition, velocity, remainingTime, verticalRotation, halfExtents, isInterior, engine))
                        {
                            Ogre::Vector3 resultantDirection = currentNormal.crossProduct(up);
                            resultantDirection.normalise();
                            clippedVelocity = velocity;
                            projectVelocity(clippedVelocity, resultantDirection);

                            // just this isn't enough sometimes. It's the same problem that causes steps to be necessary on even uphill terrain.
                            clippedVelocity += currentNormal*clippedVelocity.length()/50.0f;
                            //std::cout<< "clipped velocity: "<<clippedVelocity <<std::endl;
                        }
                        //else
                        //    std::cout<< "stepped" <<std::endl;
                    }
                    else
                        clipVelocity(clippedVelocity, currentNormal, clippedVelocity, 1.0f);
                }

                lastNormal = currentNormal;

                iterations++;
            } while(iterations < maxIterations && remainingTime != 0.0f);

            verticalVelocity = clippedVelocity.z;
            verticalVelocity -= time*400;
            physicActor->setVerticalForce(verticalVelocity);

            return newPosition;
        }
    };


    PhysicsSystem::PhysicsSystem(OEngine::Render::OgreRenderer &_rend) :
        mRender(_rend), mEngine(0), mFreeFly (true)
    {

        playerphysics = new playerMove;
        // Create physics. shapeLoader is deleted by the physic engine
        NifBullet::ManualBulletShapeLoader* shapeLoader = new NifBullet::ManualBulletShapeLoader();
        mEngine = new OEngine::Physic::PhysicEngine(shapeLoader);
        playerphysics->mEngine = mEngine;
    }

    PhysicsSystem::~PhysicsSystem()
    {
        delete mEngine;
        delete playerphysics;

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
        // OGRE to MW coordinates
        _from = btVector3(from.x, -from.z, from.y);
        _to = btVector3(to.x, -to.z, to.y);

        std::vector < std::pair <float, std::string> > results;
        /* auto */ results = mEngine->rayTest2(_from,_to);
        std::vector < std::pair <float, std::string> >::iterator i;
        for (/* auto */ i = results.begin (); i != results.end (); ++i)
            i->first *= queryDistance;
        return results;
    }

    void PhysicsSystem::setCurrentWater(bool hasWater, int waterHeight)
    {
        playerphysics->hasWater = hasWater;
        if(hasWater){
            playerphysics->waterHeight = waterHeight;
        }
        for(std::map<std::string,OEngine::Physic::PhysicActor*>::iterator it = mEngine->PhysicActorMap.begin(); it != mEngine->PhysicActorMap.end();it++)
        {
            it->second->setCurrentWater(hasWater, waterHeight);
        }

    }

    btVector3 PhysicsSystem::getRayPoint(float extent)
    {
        //get a ray pointing to the center of the viewport
        Ray centerRay = mRender.getCamera()->getCameraToViewportRay(
        mRender.getViewport()->getWidth()/2,
        mRender.getViewport()->getHeight()/2);
        btVector3 result(centerRay.getPoint(extent).x,-centerRay.getPoint(extent).z,centerRay.getPoint(extent).y);
        return result;
    }

    btVector3 PhysicsSystem::getRayPoint(float extent, float mouseX, float mouseY)
    {
        //get a ray pointing to the center of the viewport
        Ray centerRay = mRender.getCamera()->getCameraToViewportRay(mouseX, mouseY);
        btVector3 result(centerRay.getPoint(extent).x,-centerRay.getPoint(extent).z,centerRay.getPoint(extent).y);
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
        // OGRE to MW coordinates
        _from = btVector3(from.x, -from.z, from.y);
        _to = btVector3(to.x, -to.z, to.y);

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

    void PhysicsSystem::addObject (const Ptr& ptr)
    {
        std::string mesh = MWWorld::Class::get(ptr).getModel(ptr);
        Ogre::SceneNode* node = ptr.getRefData().getBaseNode();
        handleToMesh[node->getName()] = mesh;
        OEngine::Physic::RigidBody* body = mEngine->createAndAdjustRigidBody(mesh, node->getName(), node->getScale().x, node->getPosition(), node->getOrientation());
        mEngine->addRigidBody(body);
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
        Ogre::SceneNode* node = ptr.getRefData().getBaseNode();
        std::string handle = node->getName();
        Ogre::Vector3 position = node->getPosition();
        if (OEngine::Physic::RigidBody* body = mEngine->getRigidBody(handle))
        {
            // TODO very dirty hack to avoid crash during setup -> needs cleaning up to allow
            // start positions others than 0, 0, 0
            
            
            if(dynamic_cast<btBoxShape*>(body->getCollisionShape()) == NULL){
                btTransform tr = body->getWorldTransform();
                tr.setOrigin(btVector3(position.x,position.y,position.z));
                body->setWorldTransform(tr);
            }
            else{
                //For objects that contain a box shape.  
                //Do any such objects exist?  Perhaps animated objects?
                mEngine->boxAdjustExternal(handleToMesh[handle], body, node->getScale().x, position, node->getOrientation());
            }
        }
        if (OEngine::Physic::PhysicActor* act = mEngine->getCharacter(handle))
        {
            // TODO very dirty hack to avoid crash during setup -> needs cleaning up to allow
            // start positions others than 0, 0, 0
            if (handle == "player")
            {
                playerphysics->ps.origin = position;
            }
            else
            {
                act->setPosition(position);
            }
        }
    }

    void PhysicsSystem::rotateObject (const Ptr& ptr)
    {
        Ogre::SceneNode* node = ptr.getRefData().getBaseNode();
        std::string handle = node->getName();
        Ogre::Quaternion rotation = node->getOrientation();
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
    }

    void PhysicsSystem::scaleObject (const Ptr& ptr)
    {
        Ogre::SceneNode* node = ptr.getRefData().getBaseNode();
        std::string handle = node->getName();
        if(handleToMesh.find(handle) != handleToMesh.end())
        {
            removeObject(handle);
            addObject(ptr);
        }

        if (OEngine::Physic::PhysicActor* act = mEngine->getCharacter(handle))
            act->setScale(node->getScale().x);
    }

    bool PhysicsSystem::toggleCollisionMode()
    {
        playerphysics->ps.move_type = (playerphysics->ps.move_type == PM_NOCLIP ? PM_NORMAL : PM_NOCLIP);
        for(std::map<std::string,OEngine::Physic::PhysicActor*>::iterator it = mEngine->PhysicActorMap.begin(); it != mEngine->PhysicActorMap.end();it++)
        {
            if (it->first=="player")
            {
                OEngine::Physic::PhysicActor* act = it->second;

                bool cmode = act->getCollisionMode();
                if(cmode)
                {
                    act->enableCollisions(false);
                    mFreeFly = true;
                    return false;
                }
                else
                {
                    mFreeFly = false;
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
