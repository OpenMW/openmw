#include <stdexcept>

#include "physicssystem.hpp"
#include "../mwworld/ptr.hpp"
#include "../mwworld/world.hpp" // FIXME
#include <components/nifbullet/bullet_nif_loader.hpp>

#include "OgreRoot.h"
#include "OgreRenderWindow.h"
#include "OgreSceneManager.h"
#include "OgreViewport.h"
#include "OgreCamera.h"
#include "OgreTextureManager.h"



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
    void PhysicsSystem::setCurrentWater(bool hasWater, int waterHeight){
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

    bool PhysicsSystem::castRay(const Vector3& from, const Vector3& to)
    {
        btVector3 _from, _to;
        _from = btVector3(from.x, from.y, from.z);
        _to = btVector3(to.x, to.y, to.z);

        std::pair<std::string, float> result = mEngine->rayTest(_from, _to);

        return !(result.first == "");
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
            OEngine::Physic::PhysicActor* act = mEngine->getCharacter(iter->first);
			//if(iter->first == "player")
			//	std::cout << "This is player\n";
            //dirty stuff to get the camera orientation. Must be changed!

            Ogre::SceneNode *sceneNode = mRender.getScene()->getSceneNode (iter->first);
            Ogre::Vector3 dir;
            Ogre::Node* yawNode = sceneNode->getChildIterator().getNext();
            Ogre::Node* pitchNode = yawNode->getChildIterator().getNext();
			Ogre::Quaternion yawQuat = yawNode->getOrientation();
            Ogre::Quaternion pitchQuat = pitchNode->getOrientation();

            // unused
            //Ogre::Quaternion both = yawQuat * pitchQuat;

            playerphysics->ps.viewangles.x = pitchQuat.getPitch().valueDegrees();
            playerphysics->ps.viewangles.z = 0;
			playerphysics->ps.viewangles.y = yawQuat.getYaw().valueDegrees() *-1 + 90;

            if(mFreeFly)
            {
                Ogre::Vector3 dir1(iter->second.x,iter->second.z,-iter->second.y);

				pm_ref.rightmove = -dir1.x;
				pm_ref.forwardmove = dir1.z;
				pm_ref.upmove = dir1.y;


				//std::cout << "Current angle" << yawQuat.getYaw().valueDegrees() - 90<< "\n";
				//playerphysics->ps.viewangles.x = pitchQuat.getPitch().valueDegrees();
				//std::cout << "Pitch: " << yawQuat.getPitch() << "Yaw:" << yawQuat.getYaw() << "Roll: " << yawQuat.getRoll() << "\n";
                dir = 0.07*(yawQuat*pitchQuat*dir1);
            }
            else
            {

                Ogre::Quaternion quat = yawNode->getOrientation();
                Ogre::Vector3 dir1(iter->second.x,iter->second.z,-iter->second.y);

				pm_ref.rightmove = -dir1.x;
				pm_ref.forwardmove = dir1.z;
				pm_ref.upmove = dir1.y;



                dir = 0.025*(quat*dir1);
            }


            //set the walk direction
            act->setWalkDirection(btVector3(dir.x,-dir.z,dir.y));
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
                //std::cout << "ZCoord: " << coord.z << "\n";
                //std::cout << "Coord" << coord << "\n";
                //coord = Ogre::Vector3(coord.x, coord.z, coord.y);   //x, z, -y

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
            // TODO very dirty hack to avoid crash during setup -> needs cleaning up to allow
            // start positions others than 0, 0, 0
            act->setRotation(btQuaternion(rotation.x, rotation.y, rotation.z, rotation.w));
        }
    }

    void PhysicsSystem::scaleObject (const std::string& handle, float scale)
    {

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

           // unused
		   //Ogre::Vector3 objPos = node->getPosition();

         addObject (node->getName(), model, node->getOrientation(),
            node->getScale().x, node->getPosition());
     }

     void PhysicsSystem::insertActorPhysics(const MWWorld::Ptr& ptr, const std::string model){
           Ogre::SceneNode* node = ptr.getRefData().getBaseNode();
            // std::cout << "Adding node with name" << node->getName();
         addActor (node->getName(), model, node->getPosition());
     }

}
