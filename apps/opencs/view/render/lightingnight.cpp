
#include "lightingnight.hpp"

#include <OgreSceneManager.h>

CSVRender::LightingNight::LightingNight() : mSceneManager (0), mLight (0) {}

void CSVRender::LightingNight::activate (Ogre::SceneManager *sceneManager,
    const Ogre::ColourValue *defaultAmbient)
{
    mSceneManager = sceneManager;

    if (defaultAmbient)
        mSceneManager->setAmbientLight (*defaultAmbient);
    else
        mSceneManager->setAmbientLight (Ogre::ColourValue (0.2, 0.2, 0.2, 1));

    mLight = mSceneManager->createLight();
    mLight->setType (Ogre::Light::LT_DIRECTIONAL);
    mLight->setDirection (Ogre::Vector3 (0, 0, -1));
    mLight->setDiffuseColour (Ogre::ColourValue (0.2, 0.2, 0.2));
}

void CSVRender::LightingNight::deactivate()
{
    if (mLight)
    {
        mSceneManager->destroyLight (mLight);
        mLight = 0;
    }
}

void CSVRender::LightingNight::setDefaultAmbient (const Ogre::ColourValue& colour)
{
    mSceneManager->setAmbientLight (colour);
}
