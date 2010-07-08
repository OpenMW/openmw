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

MWScene::MWScene(Render::OgreRenderer &_rend)
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
}
