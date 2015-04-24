#include "renderer.hpp"

#include <SDL.h>

#include <OgreRoot.h>
#include <OgreRenderWindow.h>
#include <OgreTextureManager.h>
#include <OgreTexture.h>
#include <OgreHardwarePixelBuffer.h>
#include <OgreCamera.h>
#include <OgreViewport.h>

#include <extern/sdl4ogre/sdlwindowhelper.hpp>

#include <components/ogreinit/ogreinit.hpp>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/fstream.hpp>

#include <cassert>
#include <stdexcept>

using namespace Ogre;
using namespace OEngine::Render;

OgreRenderer::~OgreRenderer()
{
    cleanup();
    restoreWindowGammaRamp();
}

void OgreRenderer::cleanup()
{
    if (mWindow)
        Ogre::Root::getSingleton().destroyRenderTarget(mWindow);
    mWindow = NULL;

    delete mOgreInit;
    mOgreInit = NULL;

    // If we don't do this, the desktop resolution is not restored on exit
    SDL_SetWindowFullscreen(mSDLWindow, 0);

    SDL_DestroyWindow(mSDLWindow);
    mSDLWindow = NULL;
}

void OgreRenderer::update(float dt)
{
}

void OgreRenderer::screenshot(const std::string &file, const std::string& imageFormat)
{
    /*  Since Ogre uses narrow character interfaces, it does not support
        Unicode paths on Windows. Therefore we had to implement screenshot
        saving manually.
    */
    namespace bfs = boost::filesystem;
    bfs::ofstream out(bfs::path(file), std::ios::binary);

    Ogre::Image image;

    Ogre::PixelFormat pf = mWindow->suggestPixelFormat();
    int w = mWindow->getWidth();
    int h = mWindow->getHeight();

    image.loadDynamicImage(
        OGRE_ALLOC_T(Ogre::uchar, w * h * Ogre::PixelUtil::getNumElemBytes(pf), Ogre::MEMCATEGORY_GENERAL),
        w, h, 1, pf, true
    );
    mWindow->copyContentsToMemory(image.getPixelBox());

    Ogre::DataStreamPtr stream = image.encode(imageFormat);
    Ogre::MemoryDataStream *mem = dynamic_cast<Ogre::MemoryDataStream *>(stream.get());
    if (mem != 0) { // likely
        const char *ptr = reinterpret_cast<char *>(mem->getCurrentPtr());
        out.write(ptr, mem->size());
    }
    else {
        char buf[4096];
        size_t size = stream->size();
        while (size > 0) {
            size_t chunk = (size > sizeof(buf)) ? sizeof(buf) : size;
            stream->read(buf, chunk);
            out.write(buf, chunk);
            size -= chunk;
        }
    }
}

void OgreRenderer::configure(const std::string &logPath,
                            const std::string& renderSystem,
                             const std::string& rttMode
                            )
{
    mOgreInit = new OgreInit::OgreInit();
    mRoot = mOgreInit->init(logPath + "/ogre.log");

    RenderSystem* rs = mRoot->getRenderSystemByName(renderSystem);
    if (rs == 0)
        throw std::runtime_error ("RenderSystem with name " + renderSystem + " not found, make sure the plugins are loaded");
    mRoot->setRenderSystem(rs);

    if (rs->getName().find("OpenGL") != std::string::npos)
        rs->setConfigOption ("RTT Preferred Mode", rttMode);
}

void OgreRenderer::createWindow(const std::string &title, const WindowSettings& settings)
{
    assert(mRoot);
    mRoot->initialise(false);

    NameValuePairList params;
    params.insert(std::make_pair("title", title));
    params.insert(std::make_pair("FSAA", settings.fsaa));
    params.insert(std::make_pair("vsync", settings.vsync ? "true" : "false"));

    int pos_x = SDL_WINDOWPOS_CENTERED_DISPLAY(settings.screen),
        pos_y = SDL_WINDOWPOS_CENTERED_DISPLAY(settings.screen);

    if(settings.fullscreen)
    {
        pos_x = SDL_WINDOWPOS_UNDEFINED_DISPLAY(settings.screen);
        pos_y = SDL_WINDOWPOS_UNDEFINED_DISPLAY(settings.screen);
    }


    // Create an application window with the following settings:
    mSDLWindow = SDL_CreateWindow(
      "OpenMW",          // window title
      pos_x,             // initial x position
      pos_y,             // initial y position
      settings.window_x, // width, in pixels
      settings.window_y, // height, in pixels
      SDL_WINDOW_SHOWN
        | SDL_WINDOW_RESIZABLE
        | (settings.fullscreen ? SDL_WINDOW_FULLSCREEN : 0)
        | (settings.window_border ? 0 : SDL_WINDOW_BORDERLESS)
    );
    if (mSDLWindow == 0)
        throw std::runtime_error("Failed to create window: " + std::string(SDL_GetError()));

    SFO::SDLWindowHelper helper(mSDLWindow, settings.window_x, settings.window_y, title, settings.fullscreen, params);
    if (settings.icon != "")
        helper.setWindowIcon(settings.icon);
    mWindow = helper.getWindow();

    SDL_GetWindowGammaRamp(mSDLWindow, mOldSystemGammaRamp, &mOldSystemGammaRamp[256], &mOldSystemGammaRamp[512]);

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

    mScene = mRoot->createSceneManager(ST_GENERIC);

    mCamera = mScene->createCamera("cam");

    // Create one viewport, entire window
    mView = mWindow->addViewport(mCamera);
    // Alter the camera aspect ratio to match the viewport
    mCamera->setAspectRatio(Real(mView->getActualWidth()) / Real(mView->getActualHeight()));
}

void OgreRenderer::setWindowGammaContrast(float gamma, float contrast)
{
    if (mSDLWindow == NULL) return;

    Uint16 red[256], green[256], blue[256];
    for (int i = 0; i < 256; i++)
    {
        float k = i/256.0f;
        k = (k - 0.5f) * contrast + 0.5f;
        k = pow(k, 1.f/gamma);
        k *= 256;
        float value = k*256;
        if (value > 65535)  value = 65535;
        else if (value < 0) value = 0;

        red[i] = green[i] = blue[i] = static_cast<Uint16>(value);
    }
    if (SDL_SetWindowGammaRamp(mSDLWindow, red, green, blue) < 0)
        std::cout << "Couldn't set gamma: " << SDL_GetError() << std::endl;
}

void OgreRenderer::restoreWindowGammaRamp()
{
    if (mSDLWindow != NULL)
    {
        SDL_SetWindowGammaRamp(mSDLWindow, mOldSystemGammaRamp, &mOldSystemGammaRamp[256], &mOldSystemGammaRamp[512]);
    }
}

void OgreRenderer::adjustCamera(float fov, float nearClip)
{
    mCamera->setNearClipDistance(nearClip);
    mCamera->setFOVy(Degree(fov));
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

void OgreRenderer::setFov(float fov)
{
    mCamera->setFOVy(Degree(fov));
}

void OgreRenderer::windowResized(int x, int y)
{
    if (mWindowListener) {
        mWindowListener->windowResized(x,y);
    }
    else {
        mWindowWidth = x;
        mWindowHeight = y;
        mOutstandingResize = true;
    }
}

void OgreRenderer::setWindowListener(WindowSizeListener* listener)
{
    mWindowListener = listener;
    if (mOutstandingResize) {
        windowResized(mWindowWidth, mWindowHeight);
        mOutstandingResize = false;
    }
}
