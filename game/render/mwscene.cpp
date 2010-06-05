#include "mwscene.hpp"

#include <assert.h>

using namespace Render;
using namespace Ogre;

void MWScene::setup(OgreRenderer *_rend)
{
  rend = _rend;
  assert(rend);

  Root *root = rend->getRoot();
  RenderWindow *window = rend->getWindow();

  // Get the SceneManager, in this case a generic one
  sceneMgr = root->createSceneManager(ST_GENERIC);

  // Create the camera
  camera = sceneMgr->createCamera("PlayerCam");

  camera->setNearClipDistance(5);

  // Create one viewport, entire window
  vp = window->addViewport(camera);
  // Give the backround a healthy shade of green
  vp->setBackgroundColour(ColourValue(0,0.1,0));

  // Alter the camera aspect ratio to match the viewport
  camera->setAspectRatio(Real(vp->getActualWidth()) / Real(vp->getActualHeight()));
  camera->setFOVy(Degree(55));

  // Set default mipmap level (NB some APIs ignore this)
  TextureManager::getSingleton().setDefaultNumMipmaps(5);

  // Load resources
  ResourceGroupManager::getSingleton().initialiseAllResourceGroups();

  // Turn the entire scene (represented by the 'root' node) -90
  // degrees around the x axis. This makes Z go upwards, and Y go into
  // the screen (when x is to the right.) This is the orientation that
  // Morrowind uses, and it automagically makes everything work as it
  // should.
  SceneNode *rt = sceneMgr->getRootSceneNode();
  mwRoot = rt->createChildSceneNode();
  mwRoot->pitch(Degree(-90));

  // For testing
  sceneMgr->setAmbientLight(ColourValue(1,1,1));
}
