#include "sdlgraphicswindow.hpp"

#include <SDL_video.h>

#include <osg/DeleteHandler>

namespace SDLUtil
{

GraphicsWindowSDL2::~GraphicsWindowSDL2()
{
    close(true);
}


bool GraphicsWindowSDL2::setWindowDecorationImplementation(bool flag)
{
    if(!mWindow) return false;

    SDL_SetWindowBordered(mWindow, flag ? SDL_TRUE : SDL_FALSE);
    return true;
}

bool GraphicsWindowSDL2::setWindowRectangleImplementation(int x, int y, int width, int height)
{
    if(!mWindow) return false;

    SDL_SetWindowPosition(mWindow, x, y);
    SDL_SetWindowSize(mWindow, width, height);
    return true;
}

void GraphicsWindowSDL2::setWindowName(const std::string &name)
{
    if(!mWindow) return;

    SDL_SetWindowTitle(mWindow, name.c_str());
    _traits->windowName = name;
}

void GraphicsWindowSDL2::setCursor(MouseCursor mouseCursor)
{
    _traits->useCursor = false;
}


void GraphicsWindowSDL2::init()
{
    if(mValid) return;

    if(!_traits.valid())
        return;

    // getEventQueue()->setCurrentEventState(osgGA::GUIEventAdapter::getAccumulatedEventState().get());

    WindowData *inheritedWindowData = dynamic_cast<WindowData*>(_traits->inheritedWindowData.get());
    mWindow = inheritedWindowData ? inheritedWindowData->mWindow : NULL;

    mOwnsWindow = (mWindow == 0);
    if(mOwnsWindow)
    {
        OSG_NOTICE<<"Error: No SDL window provided."<<std::endl;
        return;
    }

    // SDL will change the current context when it creates a new one, so we
    // have to get the current one to be able to restore it afterward.
    SDL_Window *oldWin = SDL_GL_GetCurrentWindow();
    SDL_GLContext oldCtx = SDL_GL_GetCurrentContext();

    mContext = SDL_GL_CreateContext(mWindow);
    if(!mContext)
    {
        OSG_NOTICE<< "Error: Unable to create OpenGL graphics context: "<<SDL_GetError() <<std::endl;
        return;
    }

    setSyncToVBlank(_traits->vsync);

    SDL_GL_MakeCurrent(oldWin, oldCtx);

    mValid = true;

    getEventQueue()->syncWindowRectangleWithGraphcisContext();
}


bool GraphicsWindowSDL2::realizeImplementation()
{
    if(mRealized)
    {
        OSG_NOTICE<< "GraphicsWindowSDL2::realizeImplementation() Already realized" <<std::endl;
        return true;
    }

    if(!mValid) init();
    if(!mValid) return false;

    SDL_ShowWindow(mWindow);

    getEventQueue()->syncWindowRectangleWithGraphcisContext();

    mRealized = true;

    return true;
}

bool GraphicsWindowSDL2::makeCurrentImplementation()
{
    if(!mRealized)
    {
        OSG_NOTICE<<"Warning: GraphicsWindow not realized, cannot do makeCurrent."<<std::endl;
        return false;
    }

    return SDL_GL_MakeCurrent(mWindow, mContext)==0;
}

bool GraphicsWindowSDL2::releaseContextImplementation()
{
    if(!mRealized)
    {
        OSG_NOTICE<< "Warning: GraphicsWindow not realized, cannot do releaseContext." <<std::endl;
        return false;
    }

    return SDL_GL_MakeCurrent(NULL, NULL)==0;
}


void GraphicsWindowSDL2::closeImplementation()
{
    // OSG_NOTICE<<"Closing GraphicsWindowSDL2"<<std::endl;

    if(mContext)
        SDL_GL_DeleteContext(mContext);
    mContext = NULL;

    if(mWindow && mOwnsWindow)
        SDL_DestroyWindow(mWindow);
    mWindow = NULL;

    mValid = false;
    mRealized = false;
}

void GraphicsWindowSDL2::swapBuffersImplementation()
{
    if(!mRealized) return;

    //OSG_NOTICE<< "swapBuffersImplementation "<<this<<" "<<OpenThreads::Thread::CurrentThread()<<std::endl;

    SDL_GL_SwapWindow(mWindow);
}

void GraphicsWindowSDL2::setSyncToVBlank(bool on)
{
    SDL_GL_SetSwapInterval(on ? 1 : 0);
}

void GraphicsWindowSDL2::raiseWindow()
{
    SDL_RaiseWindow(mWindow);
}


class SDL2WindowingSystemInterface : public osg::GraphicsContext::WindowingSystemInterface
{
public:
    SDL2WindowingSystemInterface()
    {
        OSG_INFO<< "SDL2WindowingSystemInterface()" <<std::endl;
    }

    virtual ~SDL2WindowingSystemInterface()
    {
        if(osg::Referenced::getDeleteHandler())
        {
            osg::Referenced::getDeleteHandler()->setNumFramesToRetainObjects(0);
            osg::Referenced::getDeleteHandler()->flushAll();
        }

        //OSG_NOTICE<< "~SDL2WindowingSystemInterface()" <<std::endl;
    }

    virtual unsigned int getNumScreens(const osg::GraphicsContext::ScreenIdentifier&/*si*/)
    {
        return SDL_GetNumVideoDisplays();
    }

    virtual void getScreenSettings(const osg::GraphicsContext::ScreenIdentifier &si, osg::GraphicsContext::ScreenSettings &resolution)
    {
        SDL_DisplayMode mode;
        if(SDL_GetCurrentDisplayMode(si.screenNum, &mode) == 0)
        {
            int bpp = 32;
            Uint32 rmask, gmask, bmask, amask;
            SDL_PixelFormatEnumToMasks(mode.format, &bpp, &rmask, &gmask, &bmask, &amask);

            resolution.width = mode.w;
            resolution.height = mode.h;
            resolution.colorDepth = bpp;
            resolution.refreshRate = mode.refresh_rate;
        }
        else
        {
            OSG_NOTICE<< "Unable to query screen \""<<si.screenNum<<"\"." <<std::endl;
            resolution.width = 0;
            resolution.height = 0;
            resolution.colorDepth = 0;
            resolution.refreshRate = 0;
        }
    }

    virtual bool setScreenSettings(const osg::GraphicsContext::ScreenIdentifier&/*si*/, const osg::GraphicsContext::ScreenSettings&/*resolution*/)
    {
        // FIXME: SDL sets a new video mode by having the fullscreen flag on an
        // appropriately-sized window, rather than changing it 'raw'.
        return false;
    }

    virtual void enumerateScreenSettings(const osg::GraphicsContext::ScreenIdentifier &si, osg::GraphicsContext::ScreenSettingsList &resolutionList)
    {
        osg::GraphicsContext::ScreenSettingsList().swap(resolutionList);

        int num_modes = SDL_GetNumDisplayModes(si.screenNum);
        resolutionList.reserve(num_modes);

        for(int i = 0;i < num_modes;++i)
        {
            SDL_DisplayMode mode;
            if(SDL_GetDisplayMode(si.screenNum, i, &mode) != 0)
            {
                OSG_NOTICE<< "Failed to get info for display mode "<<i <<std::endl;
                continue;
            }

            int bpp = 32;
            Uint32 rmask, gmask, bmask, amask;
            if(SDL_PixelFormatEnumToMasks(mode.format, &bpp, &rmask, &gmask, &bmask, &amask) == SDL_FALSE)
            {
                OSG_NOTICE<< "Failed to get pixel format info for format ID "<<mode.format <<std::endl;
                continue;
            }

            resolutionList.push_back(osg::GraphicsContext::ScreenSettings(mode.w, mode.h, mode.refresh_rate, bpp));
        }

        if(resolutionList.empty())
        {
            OSG_NOTICE<< "SDL2WindowingSystemInterface::enumerateScreenSettings() not supported." <<std::endl;
        }
    }

    virtual osg::GraphicsContext* createGraphicsContext(osg::GraphicsContext::Traits *traits)
    {
        // No PBuffer support (you should use FBOs anyway)
        if(traits->pbuffer)
            return NULL;

        osg::ref_ptr<GraphicsWindowSDL2> window = new GraphicsWindowSDL2(traits);
        if(window->valid()) return window.release();
        return NULL;
    }
};

void setupWindowingSystemInterface()
{
    osg::GraphicsContext::setWindowingSystemInterface(new SDL2WindowingSystemInterface);
}

} // namespace TK
