#include "sdlwindowhelper.hpp"

#include <OgreStringConverter.h>
#include <OgreRoot.h>
#include <OgreTextureManager.h>

#include <GL/GL.h>

#include <SDL_syswm.h>
#include <SDL_endian.h>

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
#include "osx_utils.h"
#endif

namespace
{

bool checkMinGLVersion(std::ostream& error)
{
#ifdef WIN32 /* FIXME: Windows only until linux and mac calls to GetDC() are tested */

    // If using OpenGL we must ensure version 3.0 or higher for Ogre 1.9.
    //
    // Root::createRenderWindow() ends up calling GLStateCacheManagerImp::initializeCache()
    // which uses OpenGL 3.0 core functions glBindFramebuffer() and glBindRenderbuffer().
    // If these are not available then a null pointer exception can occur.
    //
    // Ogre 1.9 introduced StateCacheManager/OgreGLNullStateCacheManagerImp.cpp and
    // StateCacheManager/OgreGLStateCacheManagerImp.cpp.  Details from mercurial log:
    //
    //   Changeset: 4934 (fdc9a9a081f6) Many updates to the GL state cache. Also use
    //     the same CMake config to enable the state cache regardless of GL render system.
    //   Branch:    v1-9
    //   User:      David Rogers <masterfalcon@ogre3d.org>
    //   Date:      2013-09-02 00:06:32 -0500 (9 months)
    //
    // The cache managers call OpenGL framebuffer object methods. They were available as
    // extensions but became core from version 3.0.  See:
    //
    //   http://www.opengl.org/wiki/GLAPI/glBindRenderbuffer
    //   http://www.opengl.org/wiki/GLAPI/glBindFramebuffer
    //
    // Ogre recently a change to use the extension methods instead (because some earlier
    // hardware/drivers had them).  https://ogre3d.atlassian.net/browse/OGRE-402
    //
    // However, even though equivalent extensions may be available they were deprecated
    // in v3.0 and removed in v3.1 and therefore not recommended for use
    // (see: http://www.opengl.org/wiki/Framebuffer_Object). Mercurial log:
    //
    //   Changeset: 6346 (499ae0d1273b) [OGRE-402] Use extension function names to
    //     maintain compatibility with some drivers when using the GL state cache
    //   Branch:    v1-9
    //   User:      David Rogers <masterfalcon@ogre3d.org>
    //   Date:      2014-03-17 23:41:36 -0500 (2 months)
    //
    // Similar issues with old OpenGL versions reported here:
    //
    //   https://github.com/mono/MonoGame/issues/998
    //   http://www.ogre3d.org/forums/viewtopic.php?f=1&t=79698
    //
    typedef HGLRC (* WGL_CreateContext_Func) (HDC);
    typedef BOOL (* WGL_MakeCurrent_Func) (HDC, HGLRC);
    typedef BOOL (* WGL_DeleteContext_Func) (HGLRC);
    typedef const GLubyte* (* GL_GetString_Func) (unsigned int);

    WGL_CreateContext_Func wglCreateContextPtr = 0;
    WGL_MakeCurrent_Func wglMakeCurrentPtr = 0;
    WGL_DeleteContext_Func wglDeleteContextPtr = 0;
    GL_GetString_Func glGetStringPtr = 0;

    SDL_Window* sdlWin = SDL_CreateWindow(
        "temp",           // window title
        0,                // initial x position
        0,                // initial y position
        100,              // width, in pixels
        100,              // height, in pixels
        SDL_WINDOW_HIDDEN
        );
    struct SDL_SysWMinfo swmInfo;
    SDL_VERSION(&swmInfo.version);

    if (!sdlWin || (SDL_GetWindowWMInfo(sdlWin, &swmInfo) == SDL_FALSE))
    {
        error << "OpenGL version check: Couldn't get WM Info!";
        return false;
    }

    PIXELFORMATDESCRIPTOR pfd =
    {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, //Flags
        PFD_TYPE_RGBA,    //The kind of framebuffer. RGBA or palette.
        32,               //Colordepth of the framebuffer.
        0, 0, 0, 0, 0, 0,
        0,
        0,
        0,
        0, 0, 0, 0,
        24,               //Number of bits for the depthbuffer
        8,                //Number of bits for the stencilbuffer
        0,                //Number of Aux buffers in the framebuffer.
        PFD_MAIN_PLANE,
        0,
        0, 0, 0
    };

    HWND hwnd;
#ifdef WIN32
    hwnd = swmInfo.info.win.window;
#elif __MACOSX__
    hwnd = WindowContentViewHandle(swmInfo); // FIXME: not tested
#else
    hwnd = swmInfo.info.x11.window; // FIXME: not tested
#endif
    HDC hdc = GetDC(hwnd);
    if (!hdc)
    {
        error << "OpenGL version check: Failed to get device context";
        return false;
    }
    int pixelFormat = ChoosePixelFormat(hdc, &pfd);
    if (!pixelFormat || !SetPixelFormat(hdc, pixelFormat, &pfd))
    {
        error << "OpenGL version check: Failed to choose or set pixel format for temp context";
        return false;
    }

    if (SDL_GL_LoadLibrary(NULL) != 0)
    {
        error << "OpenGL version check: Failed to load GL library";
        return false;
    }
    wglCreateContextPtr = (WGL_CreateContext_Func) SDL_GL_GetProcAddress("wglCreateContext");
    wglMakeCurrentPtr = (WGL_MakeCurrent_Func) SDL_GL_GetProcAddress("wglMakeCurrent");
    wglDeleteContextPtr = (WGL_DeleteContext_Func) SDL_GL_GetProcAddress("wglDeleteContext");
    glGetStringPtr = (GL_GetString_Func) SDL_GL_GetProcAddress("glGetString");
    if (!wglCreateContextPtr || !wglMakeCurrentPtr || !wglDeleteContextPtr || !glGetStringPtr)
    {
        error << "OpenGL version check: Null GL functions";
        return false;
    }

    // Create temporary context and make sure we have support
    HGLRC tempContext = wglCreateContextPtr(hdc);
    if (!tempContext || !wglMakeCurrentPtr(hdc, tempContext))
    {
        error << "OpenGL version check: Failed to create or activate temp context";
        return false;
    }

    // Some of the code below copied from OgreGLSupport.cpp
    const GLubyte* pcVer = glGetStringPtr(GL_VERSION);
    if (!pcVer)
    {
        error << "OpenGL version check: Failed to get GL version string";
        return false;
    }

    Ogre::String tmpStr = (const char*)pcVer;
    std::cout << "OpenGL GL_VERSION: " + tmpStr << std::endl;

    // FIXME: Does not work, returns "0.0.0.0"
    //Ogre::RenderSystem* rs = Ogre::Root::getSingleton().getRenderSystem();
    //std::cout << "OpenGL Driver Version: " + rs->getDriverVersion().toString() << std::endl;

    Ogre::String glVersion = tmpStr.substr(0, tmpStr.find(" "));
    Ogre::String::size_type pos = glVersion.find(".");
    if (pos == Ogre::String::npos)
    {
        error << "OpenGL version check: Failed to parse parse GL version string";
        return false;
    }
    else
    {
        unsigned int cardFirst = ::atoi(glVersion.substr(0, pos).c_str());
        if (cardFirst < 3)
        {
            error << "OpenGL: Version " << glVersion.c_str()
                  << " detected. Version 3.0 or higer required for Ogre 1.9";
            return false;
        }
    }

    // Remove temporary context, device context and dummy window
    wglMakeCurrentPtr(NULL, NULL);
    wglDeleteContextPtr(tempContext);
    ReleaseDC(hwnd, hdc);
    SDL_DestroyWindow(sdlWin);
    SDL_GL_UnloadLibrary();
#endif
    return true;
}

}

namespace SFO
{

SDLWindowHelper::SDLWindowHelper (SDL_Window* window, int w, int h,
        const std::string& title, bool fullscreen, Ogre::NameValuePairList params)
    : mSDLWindow(window)
{
    //get the native whnd
    struct SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);

    if (SDL_GetWindowWMInfo(mSDLWindow, &wmInfo) == SDL_FALSE)
        throw std::runtime_error("Couldn't get WM Info!");

    Ogre::String winHandle;

    switch (wmInfo.subsystem)
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
        winHandle = Ogre::StringConverter::toString((unsigned long)wmInfo.info.x11.window);
        break;
#endif
    default:
        throw std::runtime_error("Unexpected WM!");
        break;
    }

    /// \todo externalWindowHandle is deprecated according to the source code. Figure out a way to get parentWindowHandle
    /// to work properly. On Linux/X11 it causes an occasional GLXBadDrawable error.
    params.insert(std::make_pair("externalWindowHandle",  winHandle));

    Ogre::RenderSystem* rs = Ogre::Root::getSingleton().getRenderSystem();
    if (rs && (rs->getName() == "OpenGL Rendering Subsystem")
           && (OGRE_VERSION_MAJOR >= 1) && (OGRE_VERSION_MINOR >= 9))
    {
        std::stringstream error;
        if (!checkMinGLVersion(error))
            throw std::runtime_error(error.str());
    }

    mWindow = Ogre::Root::getSingleton().createRenderWindow(title, w, h, fullscreen, &params);
}

void SDLWindowHelper::setWindowIcon(const std::string &name)
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

    SDL_SetWindowIcon(mSDLWindow, surface);
    SDL_FreeSurface(surface);
}

}
