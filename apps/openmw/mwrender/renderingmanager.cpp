#include "renderingmanager.hpp"

#include <assert.h>

#include "OgreRoot.h"
#include "OgreRenderWindow.h"
#include "OgreSceneManager.h"
#include "OgreViewport.h"
#include "OgreCamera.h"
#include "OgreTextureManager.h"

#include "../mwworld/world.hpp" // these includes can be removed once the static-hack is gone
#include "../mwworld/ptr.hpp"
#include <components/esm/loadstat.hpp>

#include "player.hpp"

using namespace MWRender;
using namespace Ogre;

namespace MWRender {



RenderingManager::RenderingManager (OEngine::Render::OgreRenderer& _rend, const boost::filesystem::path& resDir, OEngine::Physic::PhysicEngine* engine) :rend(_rend), mDebugging(engine)
{
	
	

	//std::cout << "ONE";
	 rend.createScene("PlayerCam", 55, 5);
	 mSkyManager = MWRender::SkyManager::create(rend.getWindow(), rend.getCamera(), resDir);

    // Set default mipmap level (NB some APIs ignore this)
    TextureManager::getSingleton().setDefaultNumMipmaps(5);

    // Load resources
    ResourceGroupManager::getSingleton().initialiseAllResourceGroups();

    // Turn the entire scene (represented by the 'root' node) -90
    // degrees around the x axis. This makes Z go upwards, and Y go into
    // the screen (when x is to the right.) This is the orientation that
    // Morrowind uses, and it automagically makes everything work as it
    // should.
	//std::cout << "TWO";
    SceneNode *rt = rend.getScene()->getRootSceneNode();
    mwRoot = rt->createChildSceneNode();
    mwRoot->pitch(Degree(-90));

    //used to obtain ingame information of ogre objects (which are faced or selected)
    mRaySceneQuery = rend.getScene()->createRayQuery(Ray());

    Ogre::SceneNode *playerNode = mwRoot->createChildSceneNode ("player");
    playerNode->pitch(Degree(90));
    Ogre::SceneNode *cameraYawNode = playerNode->createChildSceneNode();
    Ogre::SceneNode *cameraPitchNode = cameraYawNode->createChildSceneNode();
    cameraPitchNode->attachObject(rend.getCamera());

    mPlayer = new MWRender::Player (rend.getCamera(), playerNode->getName());
	//std::cout << "Three";
	
}

RenderingManager::~RenderingManager ()
{
	delete mPlayer;
    delete mSkyManager;
}

void RenderingManager::removeCell (MWWorld::Ptr::CellStore *store){

}
void RenderingManager::addObject (const MWWorld::Ptr& ptr, MWWorld::Ptr::CellStore *store){

}
void RenderingManager::removeObject (const MWWorld::Ptr& ptr, MWWorld::Ptr::CellStore *store){

}
void RenderingManager::moveObject (const MWWorld::Ptr& ptr, const Ogre::Vector3& position){

}
void RenderingManager::scaleObject (const MWWorld::Ptr& ptr, const Ogre::Vector3& scale){

}
void RenderingManager::rotateObject (const MWWorld::Ptr& ptr, const::Ogre::Quaternion& orientation){

}
void RenderingManager::moveObjectToCell (const MWWorld::Ptr& ptr, const Ogre::Vector3& position, MWWorld::Ptr::CellStore *store){

}
void RenderingManager::setPhysicsDebugRendering (bool){

}
bool RenderingManager::getPhysicsDebugRendering() const{
	return true;
}
void RenderingManager::update (float duration){


}

void RenderingManager::skyEnable ()
{
    mSkyManager->enable();
}

void RenderingManager::skyDisable ()
{
    mSkyManager->disable();
}

void RenderingManager::skySetHour (double hour)
{
    mSkyManager->setHour(hour);
}


void RenderingManager::skySetDate (int day, int month)
{
    mSkyManager->setDate(day, month);
}

int RenderingManager::skyGetMasserPhase() const
{
    return mSkyManager->getMasserPhase();
}

int RenderingManager::skyGetSecundaPhase() const
{
    return mSkyManager->getSecundaPhase();
}

void RenderingManager::skySetMoonColour (bool red)
{
    mSkyManager->setMoonColour(red);
}
bool RenderingManager::toggleRenderMode(int mode){
	return mDebugging.toggleRenderMode(mode);
}

}
