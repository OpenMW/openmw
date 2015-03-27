
#include "lightingday.hpp"

#include <OgreSceneManager.h>

CSVRender::LightingDay::LightingDay() : mSceneManager (0), mLight (0) {}

void CSVRender::LightingDay::activate (Ogre::SceneManager *sceneManager,
    const Ogre::ColourValue *defaultAmbient)
{
    mSceneManager = sceneManager;

    if (defaultAmbient)
        mSceneManager->setAmbientLight (*defaultAmbient);
    else
        mSceneManager->setAmbientLight (Ogre::ColourValue (0.7, 0.7, 0.7, 1));

    mLight = mSceneManager->createLight();
    mLight->setType (Ogre::Light::LT_DIRECTIONAL);
    mLight->setDirection (Ogre::Vector3 (0, 0, -1));
    mLight->setDiffuseColour (Ogre::ColourValue (1, 1, 1));
}

void CSVRender::LightingDay::deactivate()
{
    if (mLight)
    {
        mSceneManager->destroyLight (mLight);
        mLight = 0;
    }
}

void CSVRender::LightingDay::setDefaultAmbient (const Ogre::ColourValue& colour)
{
    mSceneManager->setAmbientLight (colour);
}
