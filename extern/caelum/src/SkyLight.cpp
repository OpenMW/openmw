/*
This file is part of Caelum.
See http://www.ogre3d.org/wiki/index.php/Caelum 

Copyright (c) 2008 Caelum team. See Contributors.txt for details.

Caelum is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Caelum is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with Caelum. If not, see <http://www.gnu.org/licenses/>.
*/

#include "CaelumPrecompiled.h"
#include "SkyLight.h"

namespace Caelum
{
    const Ogre::Real BaseSkyLight::DEFAULT_AUTO_DISABLE_THRESHOLD = 0.1;

    BaseSkyLight::BaseSkyLight (Ogre::SceneManager *sceneMgr, Ogre::SceneNode *caelumRootNode):
            mDirection(Ogre::Vector3::ZERO),
            mBodyColour(Ogre::ColourValue::White),
            mLightColour(Ogre::ColourValue::White),

            mDiffuseMultiplier(Ogre::ColourValue(1, 1, 0.9)),
            mSpecularMultiplier(Ogre::ColourValue(1, 1, 1)),
            mAmbientMultiplier(Ogre::ColourValue(0.2, 0.2, 0.2)),

            mAutoDisableLight(false),
            mAutoDisableThreshold(DEFAULT_AUTO_DISABLE_THRESHOLD),
            mForceDisableLight(false)
    {
        Ogre::String lightName = "CaelumSkyLight" + Ogre::StringConverter::toString((size_t)this);

        mMainLight = sceneMgr->createLight (lightName);
        mMainLight->setType (Ogre::Light::LT_DIRECTIONAL);

        sceneMgr->getRenderQueue()->getQueueGroup(CAELUM_RENDER_QUEUE_SUN)->setShadowsEnabled(false);

        mNode = caelumRootNode->createChildSceneNode ();
    }

    BaseSkyLight::~BaseSkyLight () {
        if (mNode) {
            static_cast<Ogre::SceneNode *>(mNode->getParent ())->removeAndDestroyChild (mNode->getName ());
            mNode = 0;
        }

        if (mMainLight) {
            mMainLight->_getManager ()->destroyLight (mMainLight);
            mMainLight = 0;
        }
    }

    void BaseSkyLight::setFarRadius (Ogre::Real radius) {
        CameraBoundElement::setFarRadius(radius);
        mRadius = radius;
    }

    void BaseSkyLight::update (
            const Ogre::Vector3& direction,
            const Ogre::ColourValue &lightColour,
            const Ogre::ColourValue &bodyColour)
    {
        setLightDirection(direction);
        setLightColour(lightColour);
        setBodyColour(bodyColour);
    }

    const Ogre::Vector3 BaseSkyLight::getLightDirection () const {
        return mDirection;
    }

    void BaseSkyLight::setLightDirection (const Ogre::Vector3 &dir) {
        mDirection = dir;
        if (mMainLight != 0) {
            mMainLight->setDirection (mNode->_getDerivedOrientation() * dir);
        }
    }

    void BaseSkyLight::setBodyColour (const Ogre::ColourValue &colour) {
        // Store this last colour
        mBodyColour = colour;
    }

    const Ogre::ColourValue BaseSkyLight::getBodyColour () const {
        return mBodyColour;
    }

    void BaseSkyLight::setLightColour (const Ogre::ColourValue &colour) {
        // Store this last colour
        mLightColour = colour;
        // Apply change
        setMainLightColour(colour);
    }

    void BaseSkyLight::setMainLightColour (const Ogre::ColourValue &colour) {
        // Set light colours.
        bool enable = shouldEnableLight (colour);
        if (enable) {
            mMainLight->setVisible(true);
            mMainLight->setDiffuseColour (colour * mDiffuseMultiplier);
            mMainLight->setSpecularColour (colour * mSpecularMultiplier);
        } else {
            mMainLight->setVisible(false);
        }
    }

    const Ogre::ColourValue BaseSkyLight::getLightColour () const {
        return mLightColour;
    }

    void BaseSkyLight::setDiffuseMultiplier (const Ogre::ColourValue &diffuse) {
        mDiffuseMultiplier = diffuse;
    }

    const Ogre::ColourValue BaseSkyLight::getDiffuseMultiplier () const {
        return mDiffuseMultiplier;
    }

    void BaseSkyLight::setSpecularMultiplier (const Ogre::ColourValue &specular) {
        mSpecularMultiplier = specular;
    }

    const Ogre::ColourValue BaseSkyLight::getSpecularMultiplier () const {
        return mSpecularMultiplier;
    }

    void BaseSkyLight::setAmbientMultiplier (const Ogre::ColourValue &ambient) {
        mAmbientMultiplier = ambient;
    }

    const Ogre::ColourValue BaseSkyLight::getAmbientMultiplier () const {
        return mAmbientMultiplier;
    }

    Ogre::Light* BaseSkyLight::getMainLight() const {
        return mMainLight;
    }

    bool BaseSkyLight::shouldEnableLight(const Ogre::ColourValue &colour) {
        if (mForceDisableLight) {
            return false;
        }
        if (mAutoDisableLight) {
            Ogre::Real sum = colour.r + colour.g + colour.b;
            return sum >= mAutoDisableThreshold;
        } else {
            return true;
        }
    }
}
