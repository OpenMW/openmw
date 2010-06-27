/*
This file is part of Caelum.
See http://www.ogre3d.org/wiki/index.php/Caelum 

Copyright (c) 2006-2008 Caelum team. See Contributors.txt for details.

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
#include "ImageStarfield.h"
#include "InternalUtilities.h"

namespace Caelum
{
    const Ogre::String ImageStarfield::STARFIELD_DOME_NAME = "CaelumStarfieldDome";
    const Ogre::String ImageStarfield::STARFIELD_MATERIAL_NAME = "CaelumStarfieldMaterial";
    const Ogre::String ImageStarfield::DEFAULT_TEXTURE_NAME = "Starfield.jpg";

    ImageStarfield::ImageStarfield
    (
        Ogre::SceneManager *sceneMgr,
        Ogre::SceneNode *caelumRootNode,
        const Ogre::String &textureName/* = DEFAULT_TEXUTRE_NAME*/
    )
    {
        mInclination = Ogre::Degree (0);

        String uniqueSuffix = "/" + InternalUtilities::pointerToString (this);

        mStarfieldMaterial.reset (InternalUtilities::checkLoadMaterialClone (STARFIELD_MATERIAL_NAME, STARFIELD_MATERIAL_NAME + uniqueSuffix));
        setTexture (textureName);

        sceneMgr->getRenderQueue ()->getQueueGroup (CAELUM_RENDER_QUEUE_STARFIELD)->setShadowsEnabled (false);

        InternalUtilities::generateSphericDome (STARFIELD_DOME_NAME, 32, InternalUtilities::DT_IMAGE_STARFIELD);

        mEntity.reset(sceneMgr->createEntity ("Caelum/StarfieldDome" + uniqueSuffix, STARFIELD_DOME_NAME));
        mEntity->setMaterialName (mStarfieldMaterial->getName());
        mEntity->setRenderQueueGroup (CAELUM_RENDER_QUEUE_STARFIELD);
        mEntity->setCastShadows (false);

        mNode.reset (caelumRootNode->createChildSceneNode ());
        mNode->attachObject (mEntity.get ());
    }

    ImageStarfield::~ImageStarfield ()
    {
    }

    void ImageStarfield::notifyCameraChanged (Ogre::Camera *cam) {
        CameraBoundElement::notifyCameraChanged (cam);
    }

    void ImageStarfield::setFarRadius (Ogre::Real radius) {
        CameraBoundElement::setFarRadius(radius);
        mNode->setScale (Ogre::Vector3::UNIT_SCALE * radius);
    }

    void ImageStarfield::setInclination (Ogre::Degree inc) {
        mInclination = inc;
    }

    void ImageStarfield::update (const float time) {
        Ogre::Quaternion orientation = Ogre::Quaternion::IDENTITY;
        orientation = orientation * Ogre::Quaternion (Ogre::Radian (mInclination + Ogre::Degree (90)), Ogre::Vector3::UNIT_X);
        orientation = orientation * Ogre::Quaternion (Ogre::Radian (-time * 2 * Ogre::Math::PI), Ogre::Vector3::UNIT_Y);

        mNode->setOrientation (orientation);
    }

    void ImageStarfield::setTexture (const Ogre::String &mapName) {
        // Update the starfield material
        mStarfieldMaterial->getBestTechnique ()->getPass (0)->getTextureUnitState (0)->setTextureName (mapName);
    }
}
