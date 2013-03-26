#include "renderer.hpp"
#include "fader.hpp"

#include "OgreRoot.h"
#include "OgreRenderWindow.h"
#include "OgreLogManager.h"
#include "OgreLog.h"
#include "OgreTextureManager.h"
#include "OgreTexture.h"
#include "OgreHardwarePixelBuffer.h"

#include <boost/filesystem.hpp>

#include <components/files/ogreplugin.hpp>

#include <cassert>
#include <cstdlib>
#include <stdexcept>

#if defined(__APPLE__) && !defined(__LP64__)
#include <Carbon/Carbon.h>
#endif

using namespace Ogre;
using namespace OEngine::Render;

#if defined(__APPLE__) && !defined(__LP64__)

CustomRoot::CustomRoot(const Ogre::String& pluginFileName, 
                    const Ogre::String& configFileName, 
                    const Ogre::String& logFileName)
: Ogre::Root(pluginFileName, configFileName, logFileName)
{}

bool CustomRoot::isQueuedEnd() const
{
    return mQueuedEnd;
}

#endif

void OgreRenderer::cleanup()
{
    delete mFader;
    mFader = NULL;

    delete mRoot;
    mRoot = NULL;

    unloadPlugins();
}

void OgreRenderer::start()
{
#if defined(__APPLE__) && !defined(__LP64__)
    // OSX Carbon Message Pump
    do {
        EventRef event = NULL;
        EventTargetRef targetWindow;
        targetWindow = GetEventDispatcherTarget();

        // If we are unable to get the target then we no longer care about events.
        if (!targetWindow) return;

        // Grab the next event while possible
        while (ReceiveNextEvent(0, NULL, kEventDurationNoWait, true, &event) == noErr)
        {
            // Dispatch the event
            SendEventToEventTarget(event, targetWindow);
            ReleaseEvent(event);
        }

        if (!mRoot->renderOneFrame()) {
            break;
        }

    } while (!mRoot->isQueuedEnd());
#else
    mRoot->startRendering();
#endif
}

void OgreRenderer::loadPlugins() 
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
}

void OgreRenderer::unloadPlugins()
{
    #ifdef ENABLE_PLUGIN_GL
    delete mGLPlugin;
    mGLPlugin = NULL;
    #endif
    #ifdef ENABLE_PLUGIN_Direct3D9
    delete mD3D9Plugin;
    mD3D9Plugin = NULL;
    #endif
    #ifdef ENABLE_PLUGIN_CgProgramManager
    delete mCgPlugin;
    mCgPlugin = NULL;
    #endif
    #ifdef ENABLE_PLUGIN_OctreeSceneManager
    delete mOctreePlugin;
    mOctreePlugin = NULL;
    #endif
    #ifdef ENABLE_PLUGIN_ParticleFX
    delete mParticleFXPlugin;
    mParticleFXPlugin = NULL;
    #endif
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
                            const std::string& renderSystem,
                             const std::string& rttMode,
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

#if defined(__APPLE__) && !defined(__LP64__)
    mRoot = new CustomRoot("", "", "");
#else
    mRoot = new Root("", "", "");
#endif

    #if defined(ENABLE_PLUGIN_GL) || defined(ENABLE_PLUGIN_Direct3D9) || defined(ENABLE_PLUGIN_CgProgramManager) || defined(ENABLE_PLUGIN_OctreeSceneManager) || defined(ENABLE_PLUGIN_ParticleFX)
    loadPlugins();
    #endif

    std::string pluginDir;
    const char* pluginEnv = getenv("OPENMW_OGRE_PLUGIN_DIR");
    if (pluginEnv)
        pluginDir = pluginEnv;
    else
    {
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
        pluginDir = ".\\";
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
        pluginDir = OGRE_PLUGIN_DIR;
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
        pluginDir = OGRE_PLUGIN_DIR_REL;
#endif
    }

    boost::filesystem::path absPluginPath = boost::filesystem::absolute(boost::filesystem::path(pluginDir));

    pluginDir = absPluginPath.string();

    Files::loadOgrePlugin(pluginDir, "RenderSystem_GL", *mRoot);
    Files::loadOgrePlugin(pluginDir, "RenderSystem_GL3Plus", *mRoot);
    Files::loadOgrePlugin(pluginDir, "RenderSystem_Direct3D9", *mRoot);
    Files::loadOgrePlugin(pluginDir, "Plugin_CgProgramManager", *mRoot);

    RenderSystem* rs = mRoot->getRenderSystemByName(renderSystem);
    if (rs == 0)
        throw std::runtime_error ("RenderSystem with name " + renderSystem + " not found, make sure the plugins are loaded");
    mRoot->setRenderSystem(rs);

    if (rs->getName().find("OpenGL") != std::string::npos)
        rs->setConfigOption ("RTT Preferred Mode", rttMode);
}

void OgreRenderer::recreateWindow(const std::string &title, const WindowSettings &settings)
{
    Ogre::ColourValue viewportBG = mView->getBackgroundColour();

    mRoot->destroyRenderTarget(mWindow);
    NameValuePairList params;
    params.insert(std::make_pair("title", title));
    params.insert(std::make_pair("FSAA", settings.fsaa));
    params.insert(std::make_pair("vsync", settings.vsync ? "true" : "false"));

    mWindow = mRoot->createRenderWindow(title, settings.window_x, settings.window_y, settings.fullscreen, &params);

    // Create one viewport, entire window
    mView = mWindow->addViewport(mCamera);
    mView->setBackgroundColour(viewportBG);

    adjustViewport();
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
    // has to be created in code with TU_DYNAMIC_WRITE_ONLY param
    // so that it can be modified at runtime.
    Ogre::TextureManager::getSingleton().createManual(
                    "transparent.png",
                    Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                    Ogre::TEX_TYPE_2D,
                    1, 1,
                    0,
                    Ogre::PF_A8R8G8B8,
                    Ogre::TU_WRITE_ONLY);
}

void OgreRenderer::createScene(const std::string& camName, float fov, float nearClip)
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

    mFader = new Fader(mScene);
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

void OgreRenderer::removeWindowEventListener(Ogre::WindowEventListener* listener)
{
    Ogre::WindowEventUtilities::removeWindowEventListener(mWindow, listener);
}

void OgreRenderer::setFov(float fov)
{
    mCamera->setFOVy(Degree(fov));
}
