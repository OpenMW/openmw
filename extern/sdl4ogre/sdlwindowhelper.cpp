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
		winHandle = Ogre::StringConverter::toString((uintptr_t)wmInfo.info.win.window);
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

#ifdef WIN32 /* FIXME: Windows only until linux and mac calls to GetDC() are tested */

    // If using OpenGL we must ensure version 3.0 or higher for Ogre 1.9.
    //
    // Ogre::Root::createRenderWindow() ends up calling Ogre::GLStateCacheManagerImp::initializeCache()
    // which uses OpenGL 3.0 core functions glBindFramebuffer() and glBindRenderbuffer().  If these are
    // not available then a null pointer exception can occur.
    //
    // Although equivalent extensions may be available they were deprecated in v3.0 and removed in
    // v3.1 and therefore not recommended for use (see: http://www.opengl.org/wiki/Framebuffer_Object)
    typedef HGLRC (* WGL_CreateContext_Func) (HDC);
    typedef BOOL (* WGL_MakeCurrent_Func) (HDC, HGLRC);
    typedef BOOL (* WGL_DeleteContext_Func) (HGLRC);
    typedef const GLubyte* (* GL_GetString_Func) (unsigned int);

    WGL_CreateContext_Func wglCreateContextPtr = 0;
    WGL_MakeCurrent_Func wglMakeCurrentPtr = 0;
    WGL_DeleteContext_Func wglDeleteContextPtr = 0;
    GL_GetString_Func glGetStringPtr = 0;

    Ogre::RenderSystem* rs = Ogre::Root::getSingleton().getRenderSystem();
    if (rs && (rs->getName() == "OpenGL Rendering Subsystem")
           && (OGRE_VERSION_MAJOR >= 1) && (OGRE_VERSION_MINOR >= 9))
    {
        SDL_Window* swin = SDL_CreateWindow(
            "temp",           // window title
            0,                // initial x position
            0,                // initial y position
            100,              // width, in pixels
            100,              // height, in pixels
            SDL_WINDOW_HIDDEN
            );
        struct SDL_SysWMinfo wInfo;
        SDL_VERSION(&wInfo.version);

        if (SDL_GetWindowWMInfo(swin, &wInfo) == SDL_FALSE)
            throw std::runtime_error("OpenGL: Couldn't get WM Info!");

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

        HDC hdc;
        HWND hwnd;
#ifdef WIN32
        hwnd = wInfo.info.win.window;
#elif __MACOSX__
        hwnd = WindowContentViewHandle(wInfo); // FIXME: not tested
#else
        hwnd = wInfo.info.x11.window; // FIXME: not tested
#endif
        hdc = GetDC(hwnd);
        int pixelFormat = ChoosePixelFormat(hdc, &pfd);
        if (!pixelFormat || !SetPixelFormat(hdc, pixelFormat, &pfd))
            throw std::runtime_error("OpenGL tmp context: Failed to choose or set pixel format");

        if (SDL_GL_LoadLibrary(NULL) != 0)
            throw std::runtime_error("OpenGL: Failed to load library");

        // Create temporary context and make sure we have support
        wglCreateContextPtr = (WGL_CreateContext_Func) SDL_GL_GetProcAddress("wglCreateContext");
        if(!wglCreateContextPtr)
            throw std::runtime_error("OpenGL: Null wglCreateContextPtr");
        HGLRC tempContext = wglCreateContextPtr(hdc);

        wglMakeCurrentPtr = (WGL_MakeCurrent_Func) SDL_GL_GetProcAddress("wglMakeCurrent");
        if (!wglMakeCurrentPtr || !tempContext || !wglMakeCurrentPtr(hdc, tempContext))
            throw std::runtime_error("OpenGL tmp context: Failed to create or activate context");

        // Only available for OpenGL 3.0 or higher...
        //int major, minor;
        //glGetIntegerv(GL_MAJOR_VERSION, &major);
        //glGetIntegerv(GL_MINOR_VERSION, &minor);

        // Most of the code below copied from OgreGLSupport.cpp
        glGetStringPtr = (GL_GetString_Func) SDL_GL_GetProcAddress("glGetString");
        if(!glGetStringPtr)
            throw std::runtime_error("OpenGL: Null glGetStringPtr");
        const GLubyte* pcVer = glGetStringPtr(GL_VERSION);
        if (!pcVer)
            throw std::runtime_error("OpenGL: Failed to get GL version string using glGetString");

        Ogre::String tmpStr = (const char*)pcVer;
        std::cout << "OpenGL GL_VERSION: " + tmpStr << std::endl;

        // Does not work...
        std::cout << "OpenGL Driver Version: " + rs->getDriverVersion().toString() << std::endl;

        Ogre::String glVersion = tmpStr.substr(0, tmpStr.find(" "));
        Ogre::String::size_type pos = glVersion.find(".");
        if (pos == Ogre::String::npos)
            throw std::runtime_error("OpenGL: Failed to parse parse GL Version string");
        else
        {
            unsigned int cardFirst = ::atoi(glVersion.substr(0, pos).c_str());
#if 0 /* FIXME: delete after testing completed */
            Ogre::String::size_type pos1 = glVersion.rfind(".");
            if (pos1 == Ogre::String::npos)
                throw std::runtime_error("OpenGL: Failed to parse parse GL Version string");
            else
            {
                unsigned int cardSecond = ::atoi(glVersion.substr(pos + 1, pos1 - (pos + 1)).c_str());
                unsigned int cardThird = ::atoi(glVersion.substr(pos1 + 1, glVersion.length()).c_str());
                std::cout << std::to_string(cardFirst)
                       +"."+ std::to_string(cardSecond)
                       +"."+ std::to_string(cardThird) << std::endl;
            }
#endif
            if (cardFirst < 3)
            {
                std::stringstream error;
                error << "OpenGL: Version " << glVersion.c_str()
                      << " detected. Version 3.0 or higer required for Ogre 1.9";
                throw std::runtime_error(error.str());
            }
        }

        // Remove temporary context, device context and dummy window
        if (tempContext)
        {
            wglDeleteContextPtr = (WGL_DeleteContext_Func) SDL_GL_GetProcAddress("wglDeleteContext");
            if(!wglDeleteContextPtr)
                throw std::runtime_error("OpenGL: Null wglDeleteContextPtr");
            wglMakeCurrentPtr(NULL, NULL);
            wglDeleteContextPtr(tempContext);
        }
        if (hdc)
            ReleaseDC(hwnd, hdc);
        if (swin)
            SDL_DestroyWindow(swin);
        SDL_GL_UnloadLibrary();
    }
#endif

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
