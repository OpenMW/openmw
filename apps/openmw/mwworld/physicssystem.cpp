#include "physicssystem.hpp"

#include <stdexcept>

#include <OgreRoot.h>
#include <OgreRenderWindow.h>
#include <OgreSceneManager.h>
#include <OgreViewport.h>
#include <OgreCamera.h>
#include <OgreTextureManager.h>

#include <components/nifbullet/bullet_nif_loader.hpp>

#include "../mwbase/world.hpp" // FIXME

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

	std::pair<std::string, float> PhysicsSystem::getFacedHandle (MWWorld::World& world)
	{
		std::string handle = "";

        //get a ray pointing to the center of the viewport
        Ray centerRay = mRender.getCamera()->getCameraToViewportRay(
        mRender.getViewport()->getWidth()/2,
        mRender.getViewport()->getHeight()/2);
        //let's avoid the capsule shape of the player.
        centerRay.setOrigin(centerRay.getOrigin() + 20*centerRay.getDirection());
        btVector3 from(centerRay.getOrigin().x,-centerRay.getOrigin().z,centerRay.getOrigin().y);
        btVector3 to(centerRay.getPoint(500).x,-centerRay.getPoint(500).z,centerRay.getPoint(500).y);

        return mEngine->rayTest(from,to);
    }

    std::vector < std::pair <float, std::string> > PhysicsSystem::getFacedObjects ()
    {
        //get a ray pointing to the center of the viewport
        Ray centerRay = mRender.getCamera()->getCameraToViewportRay(
        mRender.getViewport()->getWidth()/2,
        mRender.getViewport()->getHeight()/2);
        btVector3 from(centerRay.getOrigin().x,-centerRay.getOrigin().z,centerRay.getOrigin().y);
        btVector3 to(centerRay.getPoint(500).x,-centerRay.getPoint(500).z,centerRay.getPoint(500).y);

        return mEngine->rayTest2(from,to);
    }

    std::vector < std::pair <float, std::string> > PhysicsSystem::getFacedObjects (float mouseX, float mouseY)
    {
        Ray ray = mRender.getCamera()->getCameraToViewportRay(mouseX, mouseY);
        Ogre::Vector3 from = ray.getOrigin();
        Ogre::Vector3 to = ray.getPoint(500); /// \todo make this distance (ray length) configurable

        btVector3 _from, _to;
        // OGRE to MW coordinates
        _from = btVector3(from.x, -from.z, from.y);
        _to = btVector3(to.x, -to.z, to.y);

        return mEngine->rayTest2(_from,_to);
    }

    void PhysicsSystem::setCurrentWater(bool hasWater, int waterHeight)
    {
        playerphysics->hasWater = hasWater;
        if(hasWater){
            playerphysics->waterHeight = waterHeight;
        }

    }

    btVector3 PhysicsSystem::getRayPoint(float extent)
    {
        //get a ray pointing to the center of the viewport
        Ray centerRay = mRender.getCamera()->getCameraToViewportRay(
        mRender.getViewport()->getWidth()/2,
        mRender.getViewport()->getHeight()/2);
        btVector3 result(centerRay.getPoint(500*extent).x,-centerRay.getPoint(500*extent).z,centerRay.getPoint(500*extent).y); /// \todo make this distance (ray length) configurable
        return result;
    }

    btVector3 PhysicsSystem::getRayPoint(float extent, float mouseX, float mouseY)
    {
        //get a ray pointing to the center of the viewport
        Ray centerRay = mRender.getCamera()->getCameraToViewportRay(mouseX, mouseY);
        btVector3 result(centerRay.getPoint(500*extent).x,-centerRay.getPoint(500*extent).z,centerRay.getPoint(500*extent).y); /// \todo make this distance (ray length) configurable
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

        //set the walkdirection to 0 (no movement) for every actor)
        for(std::map<std::string,OEngine::Physic::PhysicActor*>::iterator it = mEngine->PhysicActorMap.begin(); it != mEngine->PhysicActorMap.end();it++)
        {
            OEngine::Physic::PhysicActor* act = it->second;
            act->setWalkDirection(btVector3(0,0,0));
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

            Ogre::SceneNode *sceneNode = mRender.getScene()->getSceneNode (iter->first);
            Ogre::Vector3 dir;
            Ogre::Node* yawNode = sceneNode->getChildIterator().getNext();
            Ogre::Node* pitchNode = yawNode->getChildIterator().getNext();
			Ogre::Quaternion yawQuat = yawNode->getOrientation();
            Ogre::Quaternion pitchQuat = pitchNode->getOrientation();



            playerphysics->ps.viewangles.x = pitchQuat.getPitch().valueDegrees();

			playerphysics->ps.viewangles.y = yawQuat.getYaw().valueDegrees() *-1 + 90;


            Ogre::Vector3 dir1(iter->second.x,iter->second.z,-iter->second.y);

            pm_ref.rightmove = -iter->second.x;
            pm_ref.forwardmove = -iter->second.y;
            pm_ref.upmove = iter->second.z;



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
            btVector3 newPos = it->second->getPosition();

            Ogre::Vector3 coord(newPos.x(), newPos.y(), newPos.z());
            if(it->first == "player"){

                coord = playerphysics->ps.origin;
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

    void PhysicsSystem::addObject (const std::string& handle, const std::string& mesh,
        const Ogre::Quaternion& rotation, float scale, const Ogre::Vector3& position)
    {
        handleToMesh[handle] = mesh;
        OEngine::Physic::RigidBody* body = mEngine->createRigidBody(mesh,handle,scale);
        mEngine->addRigidBody(body);
        btTransform tr;
        tr.setOrigin(btVector3(position.x,position.y,position.z));
        tr.setRotation(btQuaternion(rotation.x,rotation.y,rotation.z,rotation.w));
        body->setWorldTransform(tr);
    }

    void PhysicsSystem::addActor (const std::string& handle, const std::string& mesh,
        const Ogre::Vector3& position)
    {
        //TODO:optimize this. Searching the std::map isn't very efficient i think.
        mEngine->addCharacter(handle);
        OEngine::Physic::PhysicActor* act = mEngine->getCharacter(handle);
        act->setPosition(btVector3(position.x,position.y,position.z));
    }

    void PhysicsSystem::removeObject (const std::string& handle)
    {
        //TODO:check if actor???
        mEngine->removeCharacter(handle);
        mEngine->removeRigidBody(handle);
        mEngine->deleteRigidBody(handle);
    }

    void PhysicsSystem::moveObject (const std::string& handle, const Ogre::Vector3& position)
    {
        if (OEngine::Physic::RigidBody* body = mEngine->getRigidBody(handle))
        {
            // TODO very dirty hack to avoid crash during setup -> needs cleaning up to allow
            // start positions others than 0, 0, 0
            btTransform tr = body->getWorldTransform();
            tr.setOrigin(btVector3(position.x,position.y,position.z));
            body->setWorldTransform(tr);
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
                act->setPosition(btVector3(position.x,position.y,position.z));
            }
        }
    }

    void PhysicsSystem::rotateObject (const std::string& handle, const Ogre::Quaternion& rotation)
    {
        if (OEngine::Physic::PhysicActor* act = mEngine->getCharacter(handle))
        {
            act->setRotation(btQuaternion(rotation.x, rotation.y, rotation.z, rotation.w));
        }
        if (OEngine::Physic::RigidBody* body = mEngine->getRigidBody(handle))
        {
            body->getWorldTransform().setRotation(btQuaternion(rotation.x, rotation.y, rotation.z, rotation.w));
        }
    }

    void PhysicsSystem::scaleObject (const std::string& handle, float scale)
    {
        if(handleToMesh.find(handle) != handleToMesh.end())
        {
            btTransform transform = mEngine->getRigidBody(handle)->getWorldTransform();
            removeObject(handle);

            Ogre::Quaternion quat = Ogre::Quaternion(transform.getRotation().getW(), transform.getRotation().getX(), transform.getRotation().getY(), transform.getRotation().getZ());
            Ogre::Vector3 vec = Ogre::Vector3(transform.getOrigin().getX(), transform.getOrigin().getY(), transform.getOrigin().getZ());
            addObject(handle, handleToMesh[handle], quat, scale, vec);
        }
    }

    bool PhysicsSystem::toggleCollisionMode()
    {
		if(playerphysics->ps.move_type==PM_NOCLIP)
			playerphysics->ps.move_type=PM_NORMAL;

		else
			playerphysics->ps.move_type=PM_NOCLIP;
        for(std::map<std::string,OEngine::Physic::PhysicActor*>::iterator it = mEngine->PhysicActorMap.begin(); it != mEngine->PhysicActorMap.end();it++)
        {
            if (it->first=="player")
            {
                OEngine::Physic::PhysicActor* act = it->second;

                bool cmode = act->getCollisionMode();
                if(cmode)
                {
                    act->enableCollisions(false);
                    act->setGravity(0.);
                    act->setVerticalVelocity(0);
                    mFreeFly = true;
                    return false;
                }
                else
                {
                    mFreeFly = false;
                    act->enableCollisions(true);
                    act->setGravity(4.);
                    act->setVerticalVelocity(0);
                    return true;
                }
            }
        }

        throw std::logic_error ("can't find player");
    }

    void PhysicsSystem::insertObjectPhysics(const MWWorld::Ptr& ptr, const std::string model){

        Ogre::SceneNode* node = ptr.getRefData().getBaseNode();

        addObject(
            node->getName(),
            model,
            node->getOrientation(),
            node->getScale().x,
            node->getPosition());
    }

    void PhysicsSystem::insertActorPhysics(const MWWorld::Ptr& ptr, const std::string model){
        Ogre::SceneNode* node = ptr.getRefData().getBaseNode();
        addActor (node->getName(), model, node->getPosition());
    }

    bool PhysicsSystem::getObjectAABB(const MWWorld::Ptr &ptr, Ogre::Vector3 &min, Ogre::Vector3 &max)
    {
        std::string model = MWWorld::Class::get(ptr).getModel(ptr);
        if (model.empty()) {
            return false;
        }
        btVector3 btMin, btMax;
        float scale = ptr.getCellRef().scale;
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
