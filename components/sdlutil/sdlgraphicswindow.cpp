#include "sdlgraphicswindow.hpp"

#include <SDL_video.h>

namespace SDLUtil
{

GraphicsWindowSDL2::~GraphicsWindowSDL2()
{
    close(true);
}

GraphicsWindowSDL2::GraphicsWindowSDL2(osg::GraphicsContext::Traits *traits)
    : mWindow(0)
    , mContext(0)
    , mValid(false)
    , mRealized(false)
    , mOwnsWindow(false)
{
    _traits = traits;

    init();
    if(GraphicsWindowSDL2::valid())
    {
        setState(new osg::State);
        getState()->setGraphicsContext(this);

        if(_traits.valid() && _traits->sharedContext.valid())
        {
            getState()->setContextID(_traits->sharedContext->getState()->getContextID());
            incrementContextIDUsageCount(getState()->getContextID());
        }
        else
        {
            getState()->setContextID(osg::GraphicsContext::createNewContextID());
        }
    }
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

    WindowData *inheritedWindowData = dynamic_cast<WindowData*>(_traits->inheritedWindowData.get());
    mWindow = inheritedWindowData ? inheritedWindowData->mWindow : nullptr;

    mOwnsWindow = (mWindow == 0);
    if(mOwnsWindow)
    {
        OSG_FATAL<<"Error: No SDL window provided."<<std::endl;
        return;
    }

    // SDL will change the current context when it creates a new one, so we
    // have to get the current one to be able to restore it afterward.
    SDL_Window *oldWin = SDL_GL_GetCurrentWindow();
    SDL_GLContext oldCtx = SDL_GL_GetCurrentContext();
   
#if defined(ANDROID)
    int major = 1;
    int minor = 1;
    char *ver = getenv("OPENMW_GLES_VERSION");

    if (ver && strcmp(ver, "2") == 0) {
        major = 2;
        minor = 0;
    } else if (ver && strcmp(ver, "3") == 0) {
        major = 3;
        minor = 2;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, major);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minor);
#endif
    
    mContext = SDL_GL_CreateContext(mWindow);
    if(!mContext)
    {
        OSG_FATAL<< "Error: Unable to create OpenGL graphics context: "<<SDL_GetError() <<std::endl;
        return;
    }

    setSwapInterval(_traits->vsync);

    SDL_GL_MakeCurrent(oldWin, oldCtx);

    mValid = true;

    getEventQueue()->syncWindowRectangleWithGraphicsContext();
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

    getEventQueue()->syncWindowRectangleWithGraphicsContext();

    mRealized = true;

    return true;
}

bool GraphicsWindowSDL2::makeCurrentImplementation()
{
    if(!mRealized)
    {
        OSG_WARN<<"Warning: GraphicsWindow not realized, cannot do makeCurrent."<<std::endl;
        return false;
    }

    return SDL_GL_MakeCurrent(mWindow, mContext)==0;
}

bool GraphicsWindowSDL2::releaseContextImplementation()
{
    if(!mRealized)
    {
        OSG_WARN<< "Warning: GraphicsWindow not realized, cannot do releaseContext." <<std::endl;
        return false;
    }

    return SDL_GL_MakeCurrent(nullptr, nullptr)==0;
}


void GraphicsWindowSDL2::closeImplementation()
{
    if(mContext)
        SDL_GL_DeleteContext(mContext);
    mContext = nullptr;

    if(mWindow && mOwnsWindow)
        SDL_DestroyWindow(mWindow);
    mWindow = nullptr;

    mValid = false;
    mRealized = false;
}

void GraphicsWindowSDL2::swapBuffersImplementation()
{
    if(!mRealized) return;

    SDL_GL_SwapWindow(mWindow);
}

void GraphicsWindowSDL2::setSyncToVBlank(bool on)
{
    SDL_Window *oldWin = SDL_GL_GetCurrentWindow();
    SDL_GLContext oldCtx = SDL_GL_GetCurrentContext();

    SDL_GL_MakeCurrent(mWindow, mContext);

    setSwapInterval(on);

    SDL_GL_MakeCurrent(oldWin, oldCtx);
}

void GraphicsWindowSDL2::setSwapInterval(bool enable)
{
    if (enable)
    {
        if (SDL_GL_SetSwapInterval(-1) == -1)
        {
            OSG_NOTICE << "Adaptive vsync unsupported" << std::endl;
            if (SDL_GL_SetSwapInterval(1) == -1)
            {
                OSG_NOTICE << "Vertical synchronization unsupported, disabling" << std::endl;
                SDL_GL_SetSwapInterval(0);
            }
        }
    }
    else
    {
        SDL_GL_SetSwapInterval(0);
    }
}

void GraphicsWindowSDL2::raiseWindow()
{
    SDL_RaiseWindow(mWindow);
}

}
