#include "renderer.hpp"
#include "fader.hpp"
#include "particles.hpp"

#include <SDL.h>
#include <SDL_syswm.h>

#include "OgreRoot.h"
#include "OgreRenderWindow.h"
#include "OgreLogManager.h"
#include "OgreLog.h"
#include "OgreTextureManager.h"
#include "OgreTexture.h"
#include "OgreHardwarePixelBuffer.h"
#include <OgreParticleSystemManager.h>
#include "OgreParticleAffectorFactory.h"

#include <boost/filesystem.hpp>

#include <components/files/ogreplugin.hpp>

#include <cassert>
#include <cstdlib>
#include <stdexcept>

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
#include "osx_utils.h"
#endif

using namespace Ogre;
using namespace OEngine::Render;


#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE

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

    if (mWindowIconSurface)
        SDL_FreeSurface(mWindowIconSurface);

    // If we don't do this, the desktop resolution is not restored on exit
    SDL_SetWindowFullscreen(mSDLWindow, 0);

    SDL_DestroyWindow(mSDLWindow);
    mSDLWindow = NULL;

    unloadPlugins();
}

void OgreRenderer::start()
{
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
    // we need this custom main loop because otherwise Ogre's Carbon message pump will
    // steal input events even from our Cocoa window
    // There's no way to disable Ogre's message pump other that comment pump code in Ogre's source
    do {
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
    std::vector<Ogre::ParticleEmitterFactory*>::iterator ei;
    for(ei = mEmitterFactories.begin();ei != mEmitterFactories.end();ei++)
        OGRE_DELETE (*ei);
    mEmitterFactories.clear();

    std::vector<Ogre::ParticleAffectorFactory*>::iterator ai;
    for(ai = mAffectorFactories.begin();ai != mAffectorFactories.end();ai++)
        OGRE_DELETE (*ai);
    mAffectorFactories.clear();

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

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
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
    Files::loadOgrePlugin(pluginDir, "RenderSystem_GLES2", *mRoot);
    Files::loadOgrePlugin(pluginDir, "RenderSystem_GL3Plus", *mRoot);
    Files::loadOgrePlugin(pluginDir, "RenderSystem_Direct3D9", *mRoot);
    Files::loadOgrePlugin(pluginDir, "Plugin_CgProgramManager", *mRoot);
    Files::loadOgrePlugin(pluginDir, "Plugin_ParticleFX", *mRoot);


    Ogre::ParticleEmitterFactory *emitter;
    emitter = OGRE_NEW NifEmitterFactory();
    Ogre::ParticleSystemManager::getSingleton().addEmitterFactory(emitter);
    mEmitterFactories.push_back(emitter);


    Ogre::ParticleAffectorFactory *affector;
    affector = OGRE_NEW GrowFadeAffectorFactory();
    Ogre::ParticleSystemManager::getSingleton().addAffectorFactory(affector);
    mAffectorFactories.push_back(affector);

    affector = OGRE_NEW GravityAffectorFactory();
    Ogre::ParticleSystemManager::getSingleton().addAffectorFactory(affector);
    mAffectorFactories.push_back(affector);


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

    int pos_x = SDL_WINDOWPOS_UNDEFINED,
        pos_y = SDL_WINDOWPOS_UNDEFINED;

    if(settings.fullscreen)
    {
        SDL_Rect display_bounds;
        if(SDL_GetDisplayBounds(settings.screen, &display_bounds) != 0)
            throw std::runtime_error("Couldn't get display bounds!");
        pos_x = display_bounds.x;
        pos_y = display_bounds.y;
    }

    // Create an application window with the following settings:
    mSDLWindow = SDL_CreateWindow(
      "OpenMW",                  //    window title
      pos_x,                     //    initial x position
      pos_y,                     //    initial y position
      settings.window_x,                               //    width, in pixels
      settings.window_y,                               //    height, in pixels
      SDL_WINDOW_SHOWN
        | (settings.fullscreen ? SDL_WINDOW_FULLSCREEN : 0)
    );

    //get the native whnd
    struct SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);

    if(-1 == SDL_GetWindowWMInfo(mSDLWindow, &wmInfo))
        throw std::runtime_error("Couldn't get WM Info!");

    Ogre::String winHandle;

    switch(wmInfo.subsystem)
    {
#ifdef WIN32
    case SDL_SYSWM_WINDOWS:
        // Windows code
        winHandle = Ogre::StringConverter::toString((unsigned long)wmInfo.info.win.window);
        break;
#elif __MACOSX__
    case SDL_SYSWM_COCOA:
        //required to make OGRE play nice with our window
        params.insert(std::make_pair("macAPI", "cocoa"));
        params.insert(std::make_pair("macAPICocoaUseNSView", "true"));

        winHandle  = Ogre::StringConverter::toString(WindowContentViewHandle(wmInfo));
        break;
#else
    case SDL_SYSWM_X11:
        winHandle = Ogre::StringConverter::toString((unsigned long)wmInfo.info.x11.display);
        winHandle += ":0:";
        winHandle += Ogre::StringConverter::toString((unsigned long)wmInfo.info.x11.window);
        break;
#endif
    default:
        throw std::runtime_error("Unexpected WM!");
        break;
    }

    params.insert(std::make_pair("externalWindowHandle",  winHandle));

    mWindow = mRoot->createRenderWindow(title, settings.window_x, settings.window_y, settings.fullscreen, &params);

    // Set the window icon
    if (settings.icon != "")
    {
        mWindowIconSurface = ogreTextureToSDLSurface(settings.icon);
        SDL_SetWindowIcon(mSDLWindow, mWindowIconSurface);
    }

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
    if(mCamera != NULL)
    {
        mView->setDimensions(0, 0, 1, 1);
        mCamera->setAspectRatio(Real(mView->getActualWidth()) / Real(mView->getActualHeight()));
    }
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

SDL_Surface* OgreRenderer::ogreTextureToSDLSurface(const std::string& name)
{
    Ogre::TexturePtr texture = Ogre::TextureManager::getSingleton().load(name, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);
    if (texture.isNull())
    {
        std::stringstream error;
        error << "Window icon not found: " << name;
        throw std::runtime_error(error.str());
    }
    Ogre::Image image;
    texture->convertToImage(image);

    SDL_Surface* surface = SDL_CreateRGBSurface(0,texture->getWidth(),texture->getHeight(),32,0xFF000000,0x00FF0000,0x0000FF00,0x000000FF);

    //copy the Ogre texture to an SDL surface
    for(size_t x = 0; x < texture->getWidth(); ++x)
    {
        for(size_t y = 0; y < texture->getHeight(); ++y)
        {
            Ogre::ColourValue clr = image.getColourAt(x, y, 0);

            //set the pixel on the SDL surface to the same value as the Ogre texture's
            int bpp = surface->format->BytesPerPixel;
            /* Here p is the address to the pixel we want to set */
            Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;
            Uint32 pixel = SDL_MapRGBA(surface->format, clr.r*255, clr.g*255, clr.b*255, clr.a*255);
            switch(bpp) {
            case 1:
                *p = pixel;
                break;

            case 2:
                *(Uint16 *)p = pixel;
                break;

            case 3:
                if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
                    p[0] = (pixel >> 16) & 0xff;
                    p[1] = (pixel >> 8) & 0xff;
                    p[2] = pixel & 0xff;
                } else {
                    p[0] = pixel & 0xff;
                    p[1] = (pixel >> 8) & 0xff;
                    p[2] = (pixel >> 16) & 0xff;
                }
                break;

            case 4:
                *(Uint32 *)p = pixel;
                break;
            }
        }
    }
    return surface;
}
