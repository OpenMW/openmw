
#include "lightingbright.hpp"

#include <OgreSceneManager.h>

CSVRender::LightingBright::LightingBright() : mSceneManager (0), mLight (0) {}

void CSVRender::LightingBright::activate (Ogre::SceneManager *sceneManager,
    const Ogre::ColourValue *defaultAmbient)
{
    mSceneManager = sceneManager;

    mSceneManager->setAmbientLight (Ogre::ColourValue (1.0, 1.0, 1.0, 1));

    mLight = mSceneManager->createLight();
    mLight->setType (Ogre::Light::LT_DIRECTIONAL);
    mLight->setDirection (Ogre::Vector3 (0, 0, -1));
    mLight->setDiffuseColour (Ogre::ColourValue (1.0, 1.0, 1.0));
}

void CSVRender::LightingBright::deactivate()
{
    if (mLight)
    {
        mSceneManager->destroyLight (mLight);
        mLight = 0;
    }
}

void CSVRender::LightingBright::setDefaultAmbient (const Ogre::ColourValue& colour) {}
