/*
This file is part of Caelum.
See http://www.ogre3d.org/wiki/index.php/Caelum 

Copyright (c) 2006-2007 Caelum team. See Contributors.txt for details.

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
#include "SkyDome.h"
#include "CaelumExceptions.h"
#include "InternalUtilities.h"

namespace Caelum
{
    const Ogre::String SkyDome::SPHERIC_DOME_NAME = "CaelumSphericDome";
    const Ogre::String SkyDome::SKY_DOME_MATERIAL_NAME = "CaelumSkyDomeMaterial";

    SkyDome::SkyDome (Ogre::SceneManager *sceneMgr, Ogre::SceneNode *caelumRootNode)
    {
        String uniqueSuffix = "/" + InternalUtilities::pointerToString(this);

        // First clone material
        mMaterial.reset(InternalUtilities::checkLoadMaterialClone(SKY_DOME_MATERIAL_NAME, SKY_DOME_MATERIAL_NAME + uniqueSuffix));

        // Determine if the shader technique works.
        mShadersEnabled = mMaterial->getBestTechnique()->getPass(0)->isProgrammable();

        // Force setting haze, ensure mHazeEnabled != value.
        mHazeEnabled = true;
        setHazeEnabled(false);

        sceneMgr->getRenderQueue()->getQueueGroup(CAELUM_RENDER_QUEUE_SKYDOME)->setShadowsEnabled(false);

        // Generate dome entity.
        InternalUtilities::generateSphericDome (SPHERIC_DOME_NAME, 32, InternalUtilities::DT_SKY_DOME);
        mEntity.reset(sceneMgr->createEntity ("Caelum/SkyDome/Entity" + uniqueSuffix, SPHERIC_DOME_NAME));
        mEntity->setMaterialName (mMaterial->getName());
        mEntity->setRenderQueueGroup (CAELUM_RENDER_QUEUE_SKYDOME);
        mEntity->setCastShadows (false);

        mNode.reset(caelumRootNode->createChildSceneNode ("Caelum/SkyDome/Node" + uniqueSuffix));
        mNode->attachObject (mEntity.get());
    }

    SkyDome::~SkyDome () {
    }

    void SkyDome::notifyCameraChanged (Ogre::Camera *cam) {
        CameraBoundElement::notifyCameraChanged (cam);
    }

    void SkyDome::setFarRadius (Ogre::Real radius) {
        CameraBoundElement::setFarRadius(radius);
        mNode->setScale (Ogre::Vector3::UNIT_SCALE * radius);
    }

    void SkyDome::setSunDirection (const Ogre::Vector3& sunDir) {
        float elevation = sunDir.dotProduct (Ogre::Vector3::UNIT_Y);
        elevation = elevation * 0.5 + 0.5;
        Ogre::Pass* pass = mMaterial->getBestTechnique()->getPass(0);
        if (mShadersEnabled) {
            mParams.sunDirection.set(mParams.vpParams, sunDir);
            mParams.offset.set(mParams.fpParams, elevation);
        } else {
            Ogre::TextureUnitState* gradientsTus = pass->getTextureUnitState(0);
            gradientsTus->setTextureUScroll (elevation);
        }
    }

    void SkyDome::setHazeColour (const Ogre::ColourValue& hazeColour) {
        if (mShadersEnabled && mHazeEnabled) {
            mParams.hazeColour.set(mParams.fpParams, hazeColour);
        }    
    }

    void SkyDome::setSkyGradientsImage (const Ogre::String& gradients)
    {
        Ogre::TextureUnitState* gradientsTus =
                mMaterial->getTechnique (0)->getPass (0)->getTextureUnitState(0);

        gradientsTus->setTextureAddressingMode (Ogre::TextureUnitState::TAM_CLAMP);

        // Per 1.4 compatibility. Not tested with recent svn.
        #if OGRE_VERSION < ((1 << 16) | (3 << 8))
            gradientsTus->setTextureName (gradients, Ogre::TEX_TYPE_2D, -1, true);
        #else
            gradientsTus->setTextureName (gradients, Ogre::TEX_TYPE_2D);
            gradientsTus->setIsAlpha (true);
        #endif
    }

    void SkyDome::setAtmosphereDepthImage (const Ogre::String& atmosphereDepth)
    {
        if (!mShadersEnabled) {
            return;
        }

        Ogre::TextureUnitState* atmosphereTus =
                mMaterial->getTechnique (0)->getPass (0)->getTextureUnitState(1);

        atmosphereTus->setTextureName (atmosphereDepth, Ogre::TEX_TYPE_1D);
        atmosphereTus->setTextureAddressingMode (Ogre::TextureUnitState::TAM_CLAMP, Ogre::TextureUnitState::TAM_WRAP, Ogre::TextureUnitState::TAM_WRAP);
    }

    bool SkyDome::getHazeEnabled () const {
        return mHazeEnabled;
    }

    void SkyDome::setHazeEnabled (bool value)
    {
        if (mHazeEnabled == value) {
            return;
        }
        mHazeEnabled = value;

        if (!mShadersEnabled) {
            return;
        }

        Ogre::Pass *pass = mMaterial->getTechnique (0)->getPass (0);
        if (value) {
            pass->setFragmentProgram("CaelumSkyDomeFP");
        } else {
            pass->setFragmentProgram("CaelumSkyDomeFP_NoHaze");
        }
        mParams.setup(
                pass->getVertexProgramParameters(),
                pass->getFragmentProgramParameters());
    }

    void SkyDome::Params::setup(Ogre::GpuProgramParametersSharedPtr vpParams, Ogre::GpuProgramParametersSharedPtr fpParams)
    {
        this->fpParams = fpParams;
        this->vpParams = vpParams;
        sunDirection.bind(vpParams, "sunDirection");
        offset.bind(fpParams, "offset");
        hazeColour.bind(fpParams, "hazeColour");
    }
}
