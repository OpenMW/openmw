#ifndef GAME_RENDERING_EXTERNALRENDERING_H
#define GAME_RENDERING_EXTERNALRENDERING_H

namespace Ogre
{
    class SceneManager;
}

namespace MWRender
{
    /// \brief Base class for out of world rendering
    class ExternalRendering
    {
        public:
        
            virtual void setup (Ogre::SceneManager *sceneManager) = 0;
        
            virtual ~ExternalRendering() {}
    };
}

#endif

