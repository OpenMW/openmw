#include "sdlwindowhelper.hpp"

#include <OgreStringConverter.h>
#include <OgreRoot.h>
#include <OgreTextureManager.h>

#include <SDL_syswm.h>
#include <SDL_endian.h>
#include <stdexcept>

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
#elif ANDROID           
        case SDL_SYSWM_ANDROID:
		winHandle = Ogre::StringConverter::toString((unsigned long)wmInfo.info.android.window);
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

#ifdef ANDROID	
        SDL_GLContext context= SDL_GL_CreateContext(window);
        params.insert(std::make_pair("currentGLContext","True"));
#endif
        params.insert(std::make_pair("externalWindowHandle",  winHandle));

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
            Uint32 pixel = SDL_MapRGBA(surface->format, static_cast<Uint8>(clr.r * 255), 
                static_cast<Uint8>(clr.g * 255), static_cast<Uint8>(clr.b * 255), static_cast<Uint8>(clr.a * 255));
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
