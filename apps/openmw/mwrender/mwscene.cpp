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

MWScene::MWScene(OEngine::Render::OgreRenderer &_rend)
  : rend(_rend)
{
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

  mPlayer = new MWRender::Player (getCamera());
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

    // get all objects touched by the ray
    getRaySceneQuery()->setRay (centerRay );
    RaySceneQueryResult &result = getRaySceneQuery()->execute();

    RaySceneQueryResult::iterator nearest = result.end();

    for (RaySceneQueryResult::iterator itr = result.begin();
        itr != result.end(); itr++ )
    {
        // there seem to be omnipresent objects like the caelum sky dom,
        // the distance of these objects is always 0 so this if excludes these
        if ( itr->movable && itr->distance >= 0.1)
        {
            // horrible hack to exclude statics. this must be removed as soon as a replacement for the
            // AABB raycasting is implemented (we should not ignore statics)
            MWWorld::Ptr ptr = world.getPtrViaHandle (itr->movable->getParentSceneNode()->getName());
            if (ptr.getType()==typeid (ESM::Static))
                break;

            if ( nearest == result.end() )  //if no object is set
            {
                nearest = itr;
            }
            else if ( itr->distance < nearest->distance )
            {
                nearest = itr;
            }
        }
    }

    if ( nearest != result.end() )
    {
        handle = nearest->movable->getParentSceneNode()->getName();
        distance = nearest->distance;
    }

    return std::pair<std::string, float>(handle, distance);
}

void MWScene::doPhysics (float duration, MWWorld::World& world)
{

}

void MWScene::setMovement (const std::vector<std::string, Ogre::Vector3>& actors)
{

}

void MWScene::addObject (const std::string& handle, const std::string& mesh,
    const Ogre::Quaternion& rotation, float scale, const Ogre::Vector3& position)
{

}

void MWScene::addActor (const std::string& handle, const std::string& mesh,
    const Ogre::Vector3& position)
{

}

void MWScene::removeObject (const std::string& handle)
{

}

void MWScene::moveObject (const std::string& handle, const Ogre::Vector3& position)
{

}

void MWScene::rotateObject (const std::string& handle, const Ogre::Quaternion& rotation)
{

}

void MWScene::scaleObject (const std::string& handle, float scale)
{

}
