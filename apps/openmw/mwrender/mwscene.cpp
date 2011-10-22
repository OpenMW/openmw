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
#include <components/esm/loadstat.hpp>

#include "player.hpp"

using namespace MWRender;
using namespace Ogre;

Debugging::Debugging(OEngine::Physic::PhysicEngine* engine){
	eng = engine;
}

bool Debugging::toggleRenderMode (int mode){
	 switch (mode)
    {
        case MWWorld::World::Render_CollisionDebug:

            // TODO use a proper function instead of accessing the member variable
            // directly.
            eng->setDebugRenderingMode (!eng->isDebugCreated);
            return eng->isDebugCreated;
    }

    return false;
}

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

    Ogre::SceneNode *playerNode = mwRoot->createChildSceneNode ("player");
    playerNode->pitch(Degree(90));
    Ogre::SceneNode *cameraYawNode = playerNode->createChildSceneNode();
    Ogre::SceneNode *cameraPitchNode = cameraYawNode->createChildSceneNode();
    cameraPitchNode->attachObject(getCamera());


    mPlayer = new MWRender::Player (getCamera(), playerNode->getName());
}

MWScene::~MWScene()
{
    delete mPlayer;
}

std::pair<std::string, float> MWScene::getFacedHandle (MWWorld::World& world)
{
    std::string handle = "";

    //get a ray pointing to the center of the viewport
    Ray centerRay = getCamera()->getCameraToViewportRay(
        getViewport()->getWidth()/2,
        getViewport()->getHeight()/2);
    //let's avoid the capsule shape of the player.
    centerRay.setOrigin(centerRay.getOrigin() + 20*centerRay.getDirection());
    btVector3 from(centerRay.getOrigin().x,-centerRay.getOrigin().z,centerRay.getOrigin().y);
    btVector3 to(centerRay.getPoint(500).x,-centerRay.getPoint(500).z,centerRay.getPoint(500).y);

    return eng->rayTest(from,to);
}

bool MWScene::toggleRenderMode (int mode)
{
    switch (mode)
    {
        case MWWorld::World::Render_CollisionDebug:

            // TODO use a proper function instead of accessing the member variable
            // directly.
            eng->setDebugRenderingMode (!eng->isDebugCreated);
            return eng->isDebugCreated;
    }

    return false;
}
