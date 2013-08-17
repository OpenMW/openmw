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

#include "../mwbase/world.hpp" // FIXME
#include "../mwbase/environment.hpp"

#include <components/esm/loadgmst.hpp>
#include "../mwworld/esmstore.hpp"

#include "ptr.hpp"
#include "class.hpp"

using namespace Ogre;
namespace MWWorld
{

    static const float sMaxSlope = 60.0f;
    static const float sStepSize = 30.0f;
    // Arbitrary number. To prevent infinite loops. They shouldn't happen but it's good to be prepared.
    static const int sMaxIterations = 4;

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
            OEngine::Physic::ActorTracer tracer;
            tracer.doTrace(colobj, position, position+Ogre::Vector3(0.0f,0.0f,sStepSize), engine);
            if(tracer.mFraction == 0.0f)
                return false;

            tracer.doTrace(colobj, tracer.mEndPos, tracer.mEndPos + velocity*remainingTime, engine);
            if(tracer.mFraction < std::numeric_limits<float>::epsilon() ||
               (tracer.mFraction < 1.0f && getSlope(tracer.mPlaneNormal) > sMaxSlope))
                return false;
            float movefrac = tracer.mFraction;

            tracer.doTrace(colobj, tracer.mEndPos, tracer.mEndPos-Ogre::Vector3(0.0f,0.0f,sStepSize), engine);
            if(getSlope(tracer.mPlaneNormal) <= sMaxSlope)
            {
                // only step down onto semi-horizontal surfaces. don't step down onto the side of a house or a wall.
                position = tracer.mEndPos;
                remainingTime *= (1.0f-movefrac);
                return true;
            }

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
        static Ogre::Vector3 traceDown(const MWWorld::Ptr &ptr, OEngine::Physic::PhysicEngine *engine)
        {
            const ESM::Position &refpos = ptr.getRefData().getPosition();
            Ogre::Vector3 position(refpos.pos);

            OEngine::Physic::PhysicActor *physicActor = engine->getCharacter(ptr.getRefData().getHandle());
            if (!physicActor)
                return position;

            const Ogre::Vector3 halfExtents = physicActor->getHalfExtents();
            Ogre::Vector3 newPosition = position+Ogre::Vector3(0.0f, 0.0f, halfExtents.z);

            const int maxHeight = 200.f;
            OEngine::Physic::ActorTracer tracer;
            tracer.doTrace(physicActor->getCollisionBody(), newPosition, newPosition-Ogre::Vector3(0,0,maxHeight), engine);
            if(tracer.mFraction >= 1.0f)
                return position;

            physicActor->setOnGround(getSlope(tracer.mPlaneNormal) <= sMaxSlope);

            newPosition = tracer.mEndPos;
            newPosition.z -= halfExtents.z;
            newPosition.z += 2.0f;

            return newPosition;
        }

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
                return position + (Ogre::Quaternion(Ogre::Radian(-refpos.rot[2]), Ogre::Vector3::UNIT_Z)*
                                   Ogre::Quaternion(Ogre::Radian(-refpos.rot[1]), Ogre::Vector3::UNIT_Y)*
                                   Ogre::Quaternion(Ogre::Radian( refpos.rot[0]), Ogre::Vector3::UNIT_X)) *
                                  movement;
            }

            btCollisionObject *colobj = physicActor->getCollisionBody();
            Ogre::Vector3 halfExtents = physicActor->getHalfExtents();
            position.z += halfExtents.z;

            OEngine::Physic::ActorTracer tracer;
            bool onground = false;
            const Ogre::Quaternion orient; // Don't rotate actor collision boxes
            Ogre::Vector3 velocity;
            if(!gravity)
            {
                velocity = (Ogre::Quaternion(Ogre::Radian( -refpos.rot[2]), Ogre::Vector3::UNIT_Z)*
                            Ogre::Quaternion(Ogre::Radian( -refpos.rot[1]), Ogre::Vector3::UNIT_Y)*
                            Ogre::Quaternion(Ogre::Radian(  refpos.rot[0]), Ogre::Vector3::UNIT_X)) *
                           movement / time;
            }
            else
            {
                if(!(movement.z > 0.0f))
                {
                    tracer.doTrace(colobj, position, position-Ogre::Vector3(0,0,4), engine);
                    if(tracer.mFraction < 1.0f && getSlope(tracer.mPlaneNormal) <= sMaxSlope)
                        onground = true;
                }
                velocity = Ogre::Quaternion(Ogre::Radian(-refpos.rot[2]), Ogre::Vector3::UNIT_Z)*movement / time;
                velocity.z += physicActor->getVerticalForce();
            }

            if(onground)
            {
                // if we're on the ground, don't try to fall
                velocity.z = std::max(0.0f, velocity.z);
            }

            const Ogre::Vector3 up(0.0f, 0.0f, 1.0f);
            Ogre::Vector3 newPosition = position;
            float remainingTime = time;
            for(int iterations = 0;iterations < sMaxIterations && remainingTime > 0.01f;++iterations)
            {
                // trace to where character would go if there were no obstructions
                tracer.doTrace(colobj, newPosition, newPosition+velocity*remainingTime, engine);

                // check for obstructions
                if(tracer.mFraction >= 1.0f)
                {
                    newPosition = tracer.mEndPos;
                    remainingTime *= (1.0f-tracer.mFraction);
                    break;
                }

                //std::cout<<"angle: "<<getSlope(trace.planenormal)<<"\n";
                // We hit something. Try to step up onto it.
                if(stepMove(colobj, newPosition, velocity, remainingTime, engine))
                    onground = gravity;
                else
                {
                    // Can't move this way, try to find another spot along the plane
                    Ogre::Real movelen = velocity.normalise();
                    Ogre::Vector3 reflectdir = velocity.reflect(tracer.mPlaneNormal);
                    reflectdir.normalise();
                    velocity = slide(reflectdir, tracer.mPlaneNormal)*movelen;

                    // Do not allow sliding upward if there is gravity. Stepping will have taken
                    // care of that.
                    if(gravity)
                        velocity.z = std::min(velocity.z, 0.0f);
                }
            }

            if(onground)
            {
                tracer.doTrace(colobj, newPosition, newPosition-Ogre::Vector3(0,0,sStepSize+4.0f), engine);
                if(tracer.mFraction < 1.0f && getSlope(tracer.mPlaneNormal) <= sMaxSlope)
                    newPosition.z = tracer.mEndPos.z + 2.0f;
                else
                    onground = false;
            }

            physicActor->setOnGround(onground);
            physicActor->setVerticalForce(!onground ? velocity.z - time*627.2f : 0.0f);

            newPosition.z -= halfExtents.z;
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
        Ray ray = mRender.getCamera()->getCameraToViewportRay(0.5, 0.5);

        Ogre::Vector3 origin_ = ray.getOrigin();
        btVector3 origin(origin_.x, origin_.y, origin_.z);
        Ogre::Vector3 dir_ = ray.getDirection().normalisedCopy();
        btVector3 dir(dir_.x, dir_.y, dir_.z);

        btVector3 dest = origin + dir * queryDistance;
        std::pair <std::string, float> result;
        /*auto*/ result = mEngine->rayTest(origin, dest);
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

    std::pair<std::string,float> PhysicsSystem::getFacedHandle(const Ogre::Vector3 &origin_, const Ogre::Quaternion &orient_, float queryDistance)
    {
        btVector3 origin(origin_.x, origin_.y, origin_.z);

        std::pair<std::string,btVector3> result = mEngine->sphereTest(queryDistance,origin);
        if(result.first == "") return std::make_pair("",0);
        btVector3 a = result.second - origin;
        Ogre::Vector3 a_ = Ogre::Vector3(a.x(),a.y(),a.z());
        a_ = orient_.Inverse()*a_;
        Ogre::Vector2 a_xy = Ogre::Vector2(a_.x,a_.y);
        Ogre::Vector2 a_yz = Ogre::Vector2(a_xy.length(),a_.z);
        float axy = a_xy.angleBetween(Ogre::Vector2::UNIT_Y).valueDegrees();
        float az = a_yz.angleBetween(Ogre::Vector2::UNIT_X).valueDegrees();

        float fCombatAngleXY = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fCombatAngleXY")->getFloat();
        float fCombatAngleZ = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fCombatAngleZ")->getFloat();
        if(abs(axy) < fCombatAngleXY && abs(az) < fCombatAngleZ)
            return std::make_pair (result.first,result.second.length());
        else
            return std::make_pair("",0);
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

    std::vector<std::string> PhysicsSystem::getCollisions(const Ptr &ptr)
    {
        return mEngine->getCollisions(ptr.getRefData().getBaseNode()->getName());
    }

    Ogre::Vector3 PhysicsSystem::move(const MWWorld::Ptr &ptr, const Ogre::Vector3 &movement, float time, bool gravity)
    {
        return MovementSolver::move(ptr, movement, time, gravity, mEngine);
    }

    Ogre::Vector3 PhysicsSystem::traceDown(const MWWorld::Ptr &ptr)
    {
        return MovementSolver::traceDown(ptr, mEngine);
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
        for(std::map<std::string,OEngine::Physic::PhysicActor*>::iterator it = mEngine->mActorMap.begin(); it != mEngine->mActorMap.end();++it)
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
}
