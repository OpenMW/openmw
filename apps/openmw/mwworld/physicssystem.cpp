#include <stdexcept>

#include "physicssystem.hpp"
#include "../mwworld/ptr.hpp"
#include "../mwworld/world.hpp" // FIXME

#include "OgreRoot.h"
#include "OgreRenderWindow.h"
#include "OgreSceneManager.h"
#include "OgreViewport.h"
#include "OgreCamera.h"
#include "OgreTextureManager.h"


using namespace Ogre;
namespace MWWorld
{

    PhysicsSystem::PhysicsSystem(OEngine::Render::OgreRenderer &_rend , OEngine::Physic::PhysicEngine* physEng) :
        mRender(_rend), mEngine(physEng), mFreeFly (true)
    {

    }

    PhysicsSystem::~PhysicsSystem()
    {

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


    std::vector< std::pair<std::string, Ogre::Vector3> > PhysicsSystem::doPhysics (float duration,
        const std::vector<std::pair<std::string, Ogre::Vector3> >& actors)
    {
        //set the DebugRenderingMode. To disable it,set it to 0
        //eng->setDebugRenderingMode(1);

        //set the walkdirection to 0 (no movement) for every actor)
        for(std::map<std::string,OEngine::Physic::PhysicActor*>::iterator it = mEngine->PhysicActorMap.begin(); it != mEngine->PhysicActorMap.end();it++)
        {
            OEngine::Physic::PhysicActor* act = it->second;
            act->setWalkDirection(btVector3(0,0,0));
        }

        for (std::vector<std::pair<std::string, Ogre::Vector3> >::const_iterator iter (actors.begin());
            iter!=actors.end(); ++iter)
        {
            OEngine::Physic::PhysicActor* act = mEngine->getCharacter(iter->first);

            //dirty stuff to get the camera orientation. Must be changed!

            Ogre::SceneNode *sceneNode = mRender.getScene()->getSceneNode (iter->first);
            Ogre::Vector3 dir;
            Ogre::Node* yawNode = sceneNode->getChildIterator().getNext();
            Ogre::Node* pitchNode = yawNode->getChildIterator().getNext();
            if(mFreeFly)
            {
                Ogre::Quaternion yawQuat = yawNode->getOrientation();
                Ogre::Quaternion pitchQuat = pitchNode->getOrientation();
                Ogre::Vector3 dir1(iter->second.x,iter->second.z,-iter->second.y);
                dir = 0.07*(yawQuat*pitchQuat*dir1);
            }
            else
            {
                Ogre::Quaternion quat = yawNode->getOrientation();
                Ogre::Vector3 dir1(iter->second.x,iter->second.z,-iter->second.y);
                dir = 0.025*(quat*dir1);
            }

            //set the walk direction
            act->setWalkDirection(btVector3(dir.x,-dir.z,dir.y));
        }
        mEngine->stepSimulation(duration);

        std::vector< std::pair<std::string, Ogre::Vector3> > response;
        for(std::map<std::string,OEngine::Physic::PhysicActor*>::iterator it = mEngine->PhysicActorMap.begin(); it != mEngine->PhysicActorMap.end();it++)
        {
            btVector3 newPos = it->second->getPosition();
            Ogre::Vector3 coord(newPos.x(), newPos.y(), newPos.z());

            response.push_back(std::pair<std::string, Ogre::Vector3>(it->first, coord));
        }
        return response;
    }

    void PhysicsSystem::addObject (const std::string& handle, const std::string& mesh,
        const Ogre::Quaternion& rotation, float scale, const Ogre::Vector3& position)
    {
        OEngine::Physic::RigidBody* body = mEngine->createRigidBody(mesh,handle);
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
            act->setPosition(btVector3(position.x,position.y,position.z));
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
         addObject (node->getName(), model, node->getOrientation(),
            node->getScale().x, node->getPosition());
     }

     void PhysicsSystem::insertActorPhysics(const MWWorld::Ptr& ptr, const std::string model){
           Ogre::SceneNode* node = ptr.getRefData().getBaseNode();
            // std::cout << "Adding node with name" << node->getName();
         addActor (node->getName(), model, node->getPosition());
     }

}
