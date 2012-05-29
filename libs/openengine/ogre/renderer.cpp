#include "renderer.hpp"
#include "fader.hpp"

#include "OgreRoot.h"
#include "OgreRenderWindow.h"
#include "OgreLogManager.h"
#include "OgreLog.h"
#include "OgreTextureManager.h"
#include "OgreTexture.h"
#include "OgreHardwarePixelBuffer.h"

#include <assert.h>

using namespace Ogre;
using namespace OEngine::Render;

void OgreRenderer::cleanup()
{
    if (mFader)
        delete mFader;

    if(mRoot)
        delete mRoot;
    mRoot = NULL;
}

void OgreRenderer::start()
{
    mRoot->startRendering();
}

bool OgreRenderer::loadPlugins()
{
    #ifdef ENABLE_PLUGIN_GL
    mGLPlugin = new Ogre::GLPlugin();
    mRoot->installPlugin(mGLPlugin);
    #endif
    #ifdef ENABLE_PLUGIN_Direct3D9
    mD3D9Plugin = new Ogre::D3D9Plugin();
    mRoot->installPlugin(mD3D9Plugin);
    #endif
    #ifdef ENABLE_PLUGIN_CgProgramManager
    mCgPlugin = new Ogre::CgPlugin();
    mRoot->installPlugin(mCgPlugin);
    #endif
    #ifdef ENABLE_PLUGIN_OctreeSceneManager
    mOctreePlugin = new Ogre::OctreePlugin();
    mRoot->installPlugin(mOctreePlugin);
    #endif
    #ifdef ENABLE_PLUGIN_ParticleFX
    mParticleFXPlugin = new Ogre::ParticleFXPlugin();
    mRoot->installPlugin(mParticleFXPlugin);
    #endif
    return true;
}

void OgreRenderer::update(float dt)
{
    mFader->update(dt);
}

void OgreRenderer::screenshot(const std::string &file)
{
    mWindow->writeContentsToFile(file);
}

float OgreRenderer::getFPS()
{
    return mWindow->getLastFPS();
}

void OgreRenderer::configure(const std::string &logPath,
                            const std::string &pluginCfg,
                            const std::string& renderSystem,
                            bool _logging)
{
    // Set up logging first
    new LogManager;
    Log *log = LogManager::getSingleton().createLog(logPath + std::string("Ogre.log"));
    logging = _logging;

    if(logging)
        // Full log detail
        log->setLogDetail(LL_BOREME);
    else
        // Disable logging
        log->setDebugOutputEnabled(false);

    #if defined(ENABLE_PLUGIN_GL) || defined(ENABLE_PLUGIN_Direct3D9) || defined(ENABLE_PLUGIN_CgProgramManager) || defined(ENABLE_PLUGIN_OctreeSceneManager) || defined(ENABLE_PLUGIN_ParticleFX)
    mRoot = new Root("", "", "");
    loadPlugins();
    #else
    mRoot = new Root(pluginCfg, "", "");
    #endif

    RenderSystem* rs = mRoot->getRenderSystemByName(renderSystem);
    if (rs == 0)
        throw std::runtime_error ("RenderSystem with name " + renderSystem + " not found, make sure the plugins are loaded");
    mRoot->setRenderSystem(rs);
}

void OgreRenderer::createWindow(const std::string &title, const WindowSettings& settings)
{
    assert(mRoot);
    mRoot->initialise(false);

    NameValuePairList params;
    params.insert(std::make_pair("title", title));
    params.insert(std::make_pair("FSAA", settings.fsaa));
    params.insert(std::make_pair("vsync", settings.vsync ? "true" : "false"));

    mWindow = mRoot->createRenderWindow(title, settings.window_x, settings.window_y, settings.fullscreen, &params);

    // create the semi-transparent black background texture used by the GUI.
    // has to be created in code with TU_DYNAMIC_WRITE_ONLY_DISCARDABLE param
    // so that it can be modified at runtime. 
    mTransparentBGTexture = Ogre::TextureManager::getSingleton().createManual(
                    "transparent.png",
                    Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                    Ogre::TEX_TYPE_2D,
                    1, 1,
                    0,
                    Ogre::PF_A8R8G8B8,
                    Ogre::TU_DYNAMIC_WRITE_ONLY_DISCARDABLE);
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

    mFader = new Fader();
}

void OgreRenderer::adjustViewport()
{
    // Alter the camera aspect ratio to match the viewport
    mCamera->setAspectRatio(Real(mView->getActualWidth()) / Real(mView->getActualHeight()));
}

void OgreRenderer::setWindowEventListener(Ogre::WindowEventListener* listener)
{
	Ogre::WindowEventUtilities::addWindowEventListener(mWindow, listener);
}

void OgreRenderer::setFov(float fov)
{
    mCamera->setFOVy(Degree(fov));
}
