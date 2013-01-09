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

    void PhysicsSystem::doPhysics(float dt, const std::vector<std::pair<std::string, Ogre::Vector3> >& actors)
    {
        //set the DebugRenderingMode. To disable it,set it to 0
        //eng->setDebugRenderingMode(1);

        //set the movement keys to 0 (no movement) for every actor)
        for(std::map<std::string,OEngine::Physic::PhysicActor*>::iterator it = mEngine->PhysicActorMap.begin(); it != mEngine->PhysicActorMap.end();it++)
        {
            OEngine::Physic::PhysicActor* act = it->second;
            act->setMovement(0,0,0);
        }

        playerMove::playercmd& pm_ref = playerphysics->cmd;


        pm_ref.rightmove = 0;
        pm_ref.forwardmove = 0;
        pm_ref.upmove = 0;
       

		//playerphysics->ps.move_type = PM_NOCLIP;
        for (std::vector<std::pair<std::string, Ogre::Vector3> >::const_iterator iter (actors.begin());
            iter!=actors.end(); ++iter)
        {
            //dirty stuff to get the camera orientation. Must be changed!
            if (iter->first == "player") {
                playerphysics->ps.viewangles.x =
                    Ogre::Radian(mPlayerData.pitch).valueDegrees();



                playerphysics->ps.viewangles.y =
                    Ogre::Radian(mPlayerData.yaw).valueDegrees() + 90;

                pm_ref.rightmove = iter->second.x;
                pm_ref.forwardmove = -iter->second.y;
                pm_ref.upmove = iter->second.z;
            }
        }
        mEngine->stepSimulation(dt);
    }

    std::vector< std::pair<std::string, Ogre::Vector3> > PhysicsSystem::doPhysicsFixed (
        const std::vector<std::pair<std::string, Ogre::Vector3> >& actors)
    {
        Pmove(playerphysics);
       

        std::vector< std::pair<std::string, Ogre::Vector3> > response;
        for(std::map<std::string,OEngine::Physic::PhysicActor*>::iterator it = mEngine->PhysicActorMap.begin(); it != mEngine->PhysicActorMap.end();it++)
        {

            Ogre::Vector3 coord = it->second->getPosition();
            if(it->first == "player"){

                coord = playerphysics->ps.origin ;
                 
            }


            response.push_back(std::pair<std::string, Ogre::Vector3>(it->first, coord));
        }

        return response;
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
