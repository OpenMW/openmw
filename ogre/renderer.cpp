#include "renderer.hpp"

#include "OgreRoot.h"
#include "OgreRenderWindow.h"
#include "OgreLogManager.h"
#include "OgreLog.h"

#include <assert.h>

using namespace Ogre;
using namespace OEngine::Render;

void OgreRenderer::cleanup()
{
  if(mRoot)
    delete mRoot;
  mRoot = NULL;
}

void OgreRenderer::start()
{
  mRoot->startRendering();
}

void OgreRenderer::screenshot(const std::string &file)
{
  mWindow->writeContentsToFile(file);
}

float OgreRenderer::getFPS()
{
  return mWindow->getLastFPS();
}

bool OgreRenderer::configure(bool showConfig,
                             const std::string &cfgPath,
                             const std::string &logPath,
                             const std::string &pluginCfg,
                             bool _logging)
{
  std::string theLogFile("Ogre.log");
  std::string theCfgFile("ogre.cfg");

  theLogFile.insert(0, logPath);
  theCfgFile.insert(0, cfgPath);

  // Set up logging first
  new LogManager;
  Log *log = LogManager::getSingleton().createLog(theLogFile);
  logging = _logging;

  if(logging)
    // Full log detail
    log->setLogDetail(LL_BOREME);
  else
    // Disable logging
    log->setDebugOutputEnabled(false);

  mRoot = new Root(pluginCfg, theCfgFile, "");

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

bool OgreRenderer::configure(bool showConfig,
                             const std::string &cfgPath,
                             const std::string &pluginCfg,
                             bool _logging)
{
    return configure(showConfig, cfgPath, cfgPath, pluginCfg, _logging);
}

bool OgreRenderer::configure(bool showConfig,
                             const std::string &pluginCfg,
                             bool _logging)
{
    return configure(showConfig, "", pluginCfg, _logging);
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
