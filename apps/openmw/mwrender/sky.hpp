#ifndef _GAME_RENDER_SKY_H
#define _GAME_RENDER_SKY_H

namespace Ogre
{
    class RenderWindow;
    class Camera;
}

namespace MWRender
{
    ///
    /// Interface for the sky rendering system
    ///
    class SkyManager
    {
    public:
        static SkyManager* create (Ogre::RenderWindow* pRenderWindow, 
                                   Ogre::Camera* pCamera);
        virtual ~SkyManager() {}
    };
}

#endif // _GAME_RENDER_SKY_H
