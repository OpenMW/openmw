#ifndef SDL4OGRE_SDLWINDOWHELPER_H
#define SDL4OGRE_SDLWINDOWHELPER_H

#include <OgreRenderWindow.h>

namespace Ogre
{
	class RenderWindow;
}
struct SDL_Window;

namespace SFO
{

	/// @brief Creates an Ogre window from an SDL window and allows setting an Ogre texture as window icon
	class SDLWindowHelper
	{
	public:
		SDLWindowHelper (SDL_Window* window, int w, int h, const std::string& title, bool fullscreen, Ogre::NameValuePairList params);
		void setWindowIcon(const std::string& name);
		Ogre::RenderWindow* getWindow() { return mWindow; }

	private:
		Ogre::RenderWindow* mWindow;
		SDL_Window* mSDLWindow;
	};

}


#endif
