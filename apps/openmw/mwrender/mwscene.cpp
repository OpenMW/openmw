#include "mwscene.hpp"

#include <assert.h>

#include "OgreRoot.h"
#include "OgreRenderWindow.h"
#include "OgreSceneManager.h"
#include "OgreViewport.h"
#include "OgreCamera.h"
#include "OgreTextureManager.h"

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
}

std::pair<std::string, float> MWScene::getFacedHandle()
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
        // TODO: Check if the object can be focused (ignore walls etc..
        // in this state of openmw not possible)
        if ( itr->movable && itr->distance >= 0.1)
        {
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

