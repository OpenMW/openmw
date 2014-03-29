#ifndef OPENCS_VIEW_LIGHTING_NIGHT_H
#define OPENCS_VIEW_LIGHTING_NIGHT_H

#include "lighting.hpp"

namespace Ogre
{
    class Light;
}

namespace CSVRender
{
    class LightingNight : public Lighting
    {
            Ogre::SceneManager *mSceneManager;
            Ogre::Light *mLight;

        public:

            LightingNight();

            virtual void activate (Ogre::SceneManager *sceneManager,
                const Ogre::ColourValue *defaultAmbient = 0);

            virtual void deactivate();

            virtual void setDefaultAmbient (const Ogre::ColourValue& colour);
    };
}

#endif
