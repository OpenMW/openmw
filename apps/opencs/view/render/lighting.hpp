#ifndef OPENCS_VIEW_LIGHTING_H
#define OPENCS_VIEW_LIGHTING_H

namespace Ogre
{
    class SceneManager;
    class ColourValue;
}

namespace CSVRender
{
    class Lighting
    {
        public:

            virtual ~Lighting();

            virtual void activate (Ogre::SceneManager *sceneManager,
                const Ogre::ColourValue *defaultAmbient = 0) = 0;

            virtual void deactivate() = 0;

            virtual void setDefaultAmbient (const Ogre::ColourValue& colour) = 0;
    };
}

#endif
