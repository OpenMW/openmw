#ifndef _GAME_RENDER_SKY_H
#define _GAME_RENDER_SKY_H

#include <boost/filesystem.hpp>

namespace Ogre
{
    class RenderWindow;
    class Camera;
    class SceneNode;
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
                                   Ogre::Camera* pCamera,
                                   Ogre::SceneNode* pMwRoot,
                                   const boost::filesystem::path& resDir);
        virtual ~SkyManager() {}
        
        virtual void update(float duration) = 0;
        
        virtual void enable() = 0;
        
        virtual void disable() = 0;
        
        virtual void setHour (double hour) = 0;
        
        virtual void setDate (int day, int month) = 0;
        
        virtual int getMasserPhase() const = 0;
        
        virtual int getSecundaPhase() const = 0;
        
        virtual void setMoonColour (bool red) = 0;
    };
}

#endif // _GAME_RENDER_SKY_H
