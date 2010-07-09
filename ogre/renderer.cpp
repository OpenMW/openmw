#include "renderer.hpp"

#include "OgreRoot.h"
#include "OgreRenderWindow.h"
#include "OgreLogManager.h"
#include "OgreLog.h"

#include <assert.h>

using namespace Ogre;
using namespace Render;

void OgreRenderer::cleanup()
{
  if(mRoot)
    delete mRoot;
  mRoot = NULL;
}

void OgreRenderer::screenshot(const std::string &file)
{
  mWindow->writeContentsToFile(file);
}

bool OgreRenderer::configure(bool showConfig,
                             const std::string &pluginCfg,
                             bool _logging)
{
  // Set up logging first
  new LogManager;
  Log *log = LogManager::getSingleton().createLog("Ogre.log");
  logging = _logging;

  if(logging)
    // Full log detail
    log->setLogDetail(LL_BOREME);
  else
    // Disable logging
    log->setDebugOutputEnabled(false);

  mRoot = new Root(pluginCfg, "ogre.cfg", "");

  // Show the configuration dialog and initialise the system, if the
  // showConfig parameter is specified. The settings are stored in
  // ogre.cfg. If showConfig is false, the settings are assumed to
  // already exist in ogre.cfg.
  int result;
  if(showConfig)
    result = mRoot->showConfigDialog();
  else
    result = mRoot->restoreConfig();

  return !result;
}

void OgreRenderer::createWindow(const std::string &title)
{
  assert(mRoot);
  // Initialize OGRE window
  mWindow = mRoot->initialise(true, title, "");
}

void OgreRenderer::createScene(const std::string camName, float fov, float nearClip)
{
  assert(mRoot);
  assert(mWindow);
  // Get the SceneManager, in this case a generic one
  mScene = mRoot->createSceneManager(ST_GENERIC);

  // Create the camera
  mCamera = mScene->createCamera(camName);
  mCamera->setNearClipDistance(nearClip);
  mCamera->setFOVy(Degree(fov));
  
  // Create one viewport, entire window
  mView = mWindow->addViewport(mCamera);

  // Alter the camera aspect ratio to match the viewport
  mCamera->setAspectRatio(Real(mView->getActualWidth()) / Real(mView->getActualHeight()));
}
