#ifndef OPENCS_VIEW_LIGHTING_BRIGHT_H
#define OPENCS_VIEW_LIGHTING_BRIGHT_H

#include "lighting.hpp"

namespace Ogre
{
    class Light;
}

namespace CSVRender
{
    class LightingBright : public Lighting
    {
            Ogre::SceneManager *mSceneManager;
            Ogre::Light *mLight;

        public:

            LightingBright();

            virtual void activate (Ogre::SceneManager *sceneManager,
                const Ogre::ColourValue *defaultAmbient = 0);

            virtual void deactivate();

            virtual void setDefaultAmbient (const Ogre::ColourValue& colour);
    };
}

#endif
