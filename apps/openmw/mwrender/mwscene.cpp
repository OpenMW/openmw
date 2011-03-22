#include "mwscene.hpp"

#include <assert.h>

#include "OgreRoot.h"
#include "OgreRenderWindow.h"
#include "OgreSceneManager.h"
#include "OgreViewport.h"
#include "OgreCamera.h"
#include "OgreTextureManager.h"

#include "../mwworld/world.hpp" // these includes can be removed once the static-hack is gone
#include "../mwworld/ptr.hpp"
#include "../mwworld/doingphysics.hpp"
#include <components/esm/loadstat.hpp>

#include "player.hpp"

using namespace MWRender;
using namespace Ogre;

MWScene::MWScene(OEngine::Render::OgreRenderer &_rend , OEngine::Physic::PhysicEngine* physEng)
	: rend(_rend)
{
	eng = physEng;
	rend.createScene("PlayerCam", 55, 5);

	// Set default mipmap level (NB some APIs ignore this)
	TextureManager::getSingleton().setDefaultNumMipmaps(5);

	// Load resources
	ResourceGroupManager::getSingleton().initialiseAllResourceGroups();

	// Turn the entire scene (represented by the 'root' node) -90
	// degrees around the x axis. This makes Z go upwards, and Y go into
	// the screen (when x is to the right.) This is the orientation that
	// Morrowind uses, and it automagically makes everything work as it
	// should.
	SceneNode *rt = rend.getScene()->getRootSceneNode();
	mwRoot = rt->createChildSceneNode();
	mwRoot->pitch(Degree(-90));

	//used to obtain ingame information of ogre objects (which are faced or selected)
	mRaySceneQuery = rend.getScene()->createRayQuery(Ray());

	Ogre::SceneNode *playerNode = mwRoot->createChildSceneNode();
	playerNode->pitch(Degree(90));
	Ogre::SceneNode *cameraYawNode = playerNode->createChildSceneNode();
	Ogre::SceneNode *cameraPitchNode = cameraYawNode->createChildSceneNode();
	cameraPitchNode->attachObject(getCamera());


	mPlayer = new MWRender::Player (getCamera(), playerNode->getName());

    mFreeFly = true;
}

MWScene::~MWScene()
{
    delete mPlayer;
}

std::pair<std::string, float> MWScene::getFacedHandle (MWWorld::World& world)
{
    std::string handle = "";
    float distance = -1;

    //get a ray pointing to the center of the viewport
    Ray centerRay = getCamera()->getCameraToViewportRay(
        getViewport()->getWidth()/2,
        getViewport()->getHeight()/2);
    //let's avoid the capsule shape of the player.
    centerRay.setOrigin(centerRay.getOrigin() + 20*centerRay.getDirection());
    btVector3 from(centerRay.getOrigin().x,-centerRay.getOrigin().z,centerRay.getOrigin().y);
    btVector3 to(centerRay.getPoint(1000).x,-centerRay.getPoint(1000).z,centerRay.getPoint(1000).y);

    return eng->rayTest(from,to);
}

void MWScene::doPhysics (float duration, MWWorld::World& world,
    const std::vector<std::pair<std::string, Ogre::Vector3> >& actors)
{
    // stop changes to world from being reported back to the physics system
    MWWorld::DoingPhysics scopeGuard;

	//set the DebugRenderingMode. To disable it,set it to 0
	eng->setDebugRenderingMode(1);

    //set the walkdirection to 0 (no movement) for every actor)
    for(std::map<std::string,OEngine::Physic::PhysicActor*>::iterator it = eng->PhysicActorMap.begin(); it != eng->PhysicActorMap.end();it++)
    {
        OEngine::Physic::PhysicActor* act = it->second;
        act->setWalkDirection(btVector3(0,0,0));
    }

    for (std::vector<std::pair<std::string, Ogre::Vector3> >::const_iterator iter (actors.begin());
        iter!=actors.end(); ++iter)
    {
		OEngine::Physic::PhysicActor* act = eng->getCharacter(iter->first);

        //dirty stuff to get the camera orientation. Must be changed!

        Ogre::SceneNode *sceneNode = rend.getScene()->getSceneNode (iter->first);
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
            dir = 0.07*(quat*dir1);
        }

		//set the walk direction
		act->setWalkDirection(btVector3(dir.x,-dir.z,dir.y));
    }
	eng->stepSimulation(duration);

    for(std::map<std::string,OEngine::Physic::PhysicActor*>::iterator it = eng->PhysicActorMap.begin(); it != eng->PhysicActorMap.end();it++)
    {
        OEngine::Physic::PhysicActor* act = it->second;
		btVector3 newPos = act->getPosition();
        std::cout << newPos.x()<<newPos.y(),newPos.z();
        MWWorld::Ptr ptr = world.getPtrViaHandle (it->first);
		world.moveObject (ptr, newPos.x(), newPos.y(), newPos.z());
    }
}

void MWScene::addObject (const std::string& handle, const std::string& mesh,
    const Ogre::Quaternion& rotation, float scale, const Ogre::Vector3& position)
{
	OEngine::Physic::RigidBody* body = eng->createRigidBody(mesh,handle);
	eng->addRigidBody(body);
	btTransform tr;
	tr.setOrigin(btVector3(position.x,position.y,position.z));
	tr.setRotation(btQuaternion(rotation.x,rotation.y,rotation.z,rotation.w));
	body->setWorldTransform(tr);
}

void MWScene::addActor (const std::string& handle, const std::string& mesh,
    const Ogre::Vector3& position)
{
	//TODO:optimize this. Searching the std::map isn't very efficient i think.
	eng->addCharacter(handle);
	OEngine::Physic::PhysicActor* act = eng->getCharacter(handle);
	act->setPosition(btVector3(position.x,position.y,position.z));
}

void MWScene::removeObject (const std::string& handle)
{
	//TODO:check if actor???
	eng->removeRigidBody(handle);
	eng->deleteRigidBody(handle);
}

void MWScene::moveObject (const std::string& handle, const Ogre::Vector3& position, bool updatePhysics)
{
    rend.getScene()->getSceneNode(handle)->setPosition(position);

    if(updatePhysics)//TODO: is it an actor?
    {
        if (OEngine::Physic::RigidBody* body = eng->getRigidBody(handle))
        {
            // TODO very dirty hack to avoid crash during setup -> needs cleaning up to allow
            // start positions others than 0, 0, 0
            btTransform tr = body->getWorldTransform();
            tr.setOrigin(btVector3(position.x,position.y,position.z));
            body->setWorldTransform(tr);
        }
    }
}

void MWScene::rotateObject (const std::string& handle, const Ogre::Quaternion& rotation)
{
}

void MWScene::scaleObject (const std::string& handle, float scale)
{

}

void MWScene::toggleCollisionMode()
{
    for(std::map<std::string,OEngine::Physic::PhysicActor*>::iterator it = eng->PhysicActorMap.begin(); it != eng->PhysicActorMap.end();it++)
    {
        OEngine::Physic::PhysicActor* act = it->second;
        bool cmode = act->getCollisionMode();
        if(cmode)
        {
            act->enableCollisions(false);
            act->setGravity(0.);
            act->setVerticalVelocity(0);
            mFreeFly = true;
        }
        else
        {
            mFreeFly = false;
            act->enableCollisions(true);
            act->setGravity(10.);
            act->setVerticalVelocity(0);
        }
    }
}
