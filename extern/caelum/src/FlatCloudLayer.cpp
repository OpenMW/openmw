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
#include "FlatCloudLayer.h"
#include "CaelumExceptions.h"
#include "InternalUtilities.h"

namespace Caelum
{
	FlatCloudLayer::FlatCloudLayer(
            Ogre::SceneManager *sceneMgr,
			Ogre::SceneNode *cloudRoot)
	{
        Ogre::String uniqueSuffix = InternalUtilities::pointerToString(this);

		// Clone material
		mMaterial.reset(InternalUtilities::checkLoadMaterialClone ("CaelumLayeredClouds", "Caelum/FlatCloudLayer/Material" + uniqueSuffix));

        mParams.setup(
                mMaterial->getTechnique(0)->getPass(0)->getVertexProgramParameters(),
                mMaterial->getTechnique(0)->getPass(0)->getFragmentProgramParameters());

        // Create the scene node.
		mSceneMgr = sceneMgr;
		mNode.reset(cloudRoot->createChildSceneNode());
		mNode->setPosition(Ogre::Vector3(0, 0, 0));

        // Noise texture names are fixed.
        mNoiseTextureNames.clear();
        mNoiseTextureNames.push_back("noise1.dds");
        mNoiseTextureNames.push_back("noise2.dds");
        mNoiseTextureNames.push_back("noise3.dds");
        mNoiseTextureNames.push_back("noise4.dds");

        // Invalid; will reset on first opportunity.
        mCurrentTextureIndex = -1;

        // By default height is 0; the user is expected to change this.
		setHeight(0);		

        // Reset parameters. This is relied upon to initialize most fields.
        this->reset();

        // Ensure geometry; don't wait for first update.
        this->_ensureGeometry();
	}
	
	FlatCloudLayer::~FlatCloudLayer()
    {
		mSceneMgr = 0;

        // Rely on PrivatePtr for everything interesting.
	}	

    void FlatCloudLayer::_invalidateGeometry () {
        mMeshDirty = true;
    }

    void FlatCloudLayer::_ensureGeometry ()
    {
        if (!mMeshDirty) {
            return;
        }

        // Generate unique names based on pointer.
        Ogre::String uniqueId = Ogre::StringConverter::toString((size_t)this);
        Ogre::String planeMeshName = "Caelum/FlatCloudLayer/Plane/" + uniqueId;
        Ogre::String entityName = "Caelum/FlatCloudLayer/Entity/" + uniqueId;

        // Cleanup first. Entity references mesh so it must be destroyed first.
        mEntity.reset();
        mMesh.reset();

        /*
        Ogre::LogManager::getSingleton().logMessage(
                "Creating cloud layer mesh " +
                Ogre::StringConverter::toString(mMeshWidthSegments) + "x" +
                Ogre::StringConverter::toString(mMeshHeightSegments) + " segments");
         */

        // Recreate mesh.
        Ogre::Plane meshPlane(
                Ogre::Vector3(1, 1, 0),
                Ogre::Vector3(1, 1, 1),
                Ogre::Vector3(0, 1, 1));
		mMesh.reset(Ogre::MeshManager::getSingleton().createPlane(
                planeMeshName, Caelum::RESOURCE_GROUP_NAME, meshPlane,
                mMeshWidth, mMeshHeight,
			    mMeshWidthSegments, mMeshHeightSegments,
                false, 1,
                1.0f, 1.0f,
			    Ogre::Vector3::UNIT_X));

        // Recreate entity.
		mEntity.reset(mSceneMgr->createEntity(entityName, mMesh->getName()));
		mEntity->setMaterialName(mMaterial->getName());

        // Reattach entity.
		mNode->attachObject(mEntity.get());

        // Mark done.
        mMeshDirty = false;
    }

    void FlatCloudLayer::setMeshParameters (
            Real meshWidth, Real meshHeight,
            int meshWidthSegments, int meshHeightSegments)
    {
        bool invalidate =
                (mMeshWidthSegments != meshWidthSegments) ||
                (mMeshHeightSegments != meshHeightSegments) ||
                (abs(mMeshWidth - meshWidth) > 0.001) ||
                (abs(mMeshHeight - meshHeight) > 0.001);
        mMeshWidth = meshWidth;
        mMeshHeight = meshHeight;
        mMeshWidthSegments = meshWidthSegments;
        mMeshHeightSegments = meshHeightSegments;
        if (invalidate) {
            _invalidateGeometry();
        }
    }

	void FlatCloudLayer::reset()
    {
        _invalidateGeometry ();
        setMeshParameters(10000000, 10000000, 10, 10);

        assert (mCloudCoverLookup.get() == 0);
        setCloudCoverLookup ("CloudCoverLookup.png");
		setCloudCover (0.3);
        setCloudCoverVisibilityThreshold (0.001);

		setCloudMassOffset (Ogre::Vector2(0, 0));
		setCloudDetailOffset (Ogre::Vector2(0, 0));
		setCloudBlendTime (3600 * 24);
		setCloudBlendPos (0.5);

		setCloudSpeed (Ogre::Vector2(0.000005, -0.000009));

		setCloudUVFactor (150);
		setHeightRedFactor (100000);

        setFadeDistances (10000, 140000);
        setFadeDistMeasurementVector (Ogre::Vector3(0, 1, 1));

        setSunDirection (Ogre::Vector3::UNIT_Y);
        setFogColour (Ogre::ColourValue::Black);
        setSunLightColour (Ogre::ColourValue::White);
        setSunSphereColour (Ogre::ColourValue::White);
    }

	void FlatCloudLayer::update (
            Ogre::Real timePassed,
			const Ogre::Vector3 &sunDirection,
			const Ogre::ColourValue &sunLightColour,
			const Ogre::ColourValue &fogColour,
			const Ogre::ColourValue &sunSphereColour)
	{
        // Advance animation
        advanceAnimation (timePassed);

	    // Set parameters.
		setSunDirection (sunDirection);
		setSunLightColour (sunLightColour);
		setSunSphereColour (sunSphereColour);
		setFogColour (fogColour);

        this->_ensureGeometry();

        this->_updateVisibilityThreshold();
	}	

	void FlatCloudLayer::_updateVisibilityThreshold ()
    {
        if (!mEntity.isNull()) {
            mEntity->setVisible (getCloudCover () > this->getCloudCoverVisibilityThreshold ());
        }
    }

	void FlatCloudLayer::advanceAnimation (Ogre::Real timePassed)
    {
	    // Move clouds.
		setCloudMassOffset(mCloudMassOffset + timePassed * mCloudSpeed);
		setCloudDetailOffset(mCloudDetailOffset - timePassed * mCloudSpeed);

		// Animate cloud blending.
        setCloudBlendPos (getCloudBlendPos () + timePassed / mCloudBlendTime);
    }

    void FlatCloudLayer::Params::setup(Ogre::GpuProgramParametersSharedPtr vpParams, Ogre::GpuProgramParametersSharedPtr fpParams)
    {
        this->vpParams = vpParams;
        this->fpParams = fpParams;
        cloudCoverageThreshold.bind(fpParams, "cloudCoverageThreshold");
        cloudMassOffset.bind(fpParams, "cloudMassOffset");
        cloudDetailOffset.bind(fpParams, "cloudDetailOffset");
        cloudMassBlend.bind(fpParams, "cloudMassBlend");
        vpSunDirection.bind(vpParams, "sunDirection");
        fpSunDirection.bind(fpParams, "sunDirection");
        sunLightColour.bind(fpParams, "sunLightColour");
        sunSphereColour.bind(fpParams, "sunSphereColour");
        fogColour.bind(fpParams, "fogColour");
        layerHeight.bind(fpParams, "layerHeight");
        cloudUVFactor.bind(fpParams, "cloudUVFactor");
        heightRedFactor.bind(fpParams, "heightRedFactor");
        nearFadeDist.bind(fpParams, "nearFadeDist");
        farFadeDist.bind(fpParams, "farFadeDist");
        fadeDistMeasurementVector.bind(fpParams, "fadeDistMeasurementVector");
    }

	void FlatCloudLayer::setCloudCoverLookup (const Ogre::String& fileName) {
        mCloudCoverLookup.reset(0);
        mCloudCoverLookup.reset(new Ogre::Image());
        mCloudCoverLookup->load(fileName, RESOURCE_GROUP_NAME);

        mCloudCoverLookupFileName = fileName;
    }

    void FlatCloudLayer::disableCloudCoverLookup () {
        mCloudCoverLookup.reset (0);
        mCloudCoverLookupFileName.clear ();
    }

    const Ogre::String FlatCloudLayer::getCloudCoverLookupFileName () const {
        return mCloudCoverLookupFileName;
    }

	void FlatCloudLayer::setCloudCoverVisibilityThreshold(const Ogre::Real value) {
        mCloudCoverVisibilityThreshold = value;
        _updateVisibilityThreshold();
    }

	void FlatCloudLayer::setCloudCover(const Ogre::Real cloudCover) {
        mCloudCover = cloudCover;
		float cloudCoverageThreshold = 0;
        if (mCloudCoverLookup.get() != 0) {
			cloudCoverageThreshold = InternalUtilities::getInterpolatedColour(cloudCover, 1, mCloudCoverLookup.get(), false).r;
        } else {
            cloudCoverageThreshold = 1 - cloudCover;   
        }
		mParams.cloudCoverageThreshold.set(mParams.fpParams, cloudCoverageThreshold);
        _updateVisibilityThreshold();
	}

	void FlatCloudLayer::setCloudMassOffset(const Ogre::Vector2 &cloudMassOffset) {
		mCloudMassOffset = cloudMassOffset;		
		mParams.cloudMassOffset.set(mParams.fpParams, Ogre::Vector3(cloudMassOffset.x,cloudMassOffset.y,0));		
	}

	void FlatCloudLayer::setCloudDetailOffset(const Ogre::Vector2 &cloudDetailOffset) {
		mCloudDetailOffset = cloudDetailOffset;
		mParams.cloudDetailOffset.set(mParams.fpParams, Ogre::Vector3(cloudDetailOffset.x,cloudDetailOffset.y,0));		
	}

	void FlatCloudLayer::setCloudBlendTime(const Ogre::Real value) {
		mCloudBlendTime = value;
	}

    Ogre::Real FlatCloudLayer::getCloudBlendTime () const {
        return mCloudBlendTime;
    }

    void FlatCloudLayer::setCloudBlendPos (const Ogre::Real value)
    {
        mCloudBlendPos = value;
        int textureCount = static_cast<int>(mNoiseTextureNames.size());

        // Convert to int and bring to [0, textureCount)
        int currentTextureIndex = static_cast<int>(floor(mCloudBlendPos));
        currentTextureIndex = ((currentTextureIndex % textureCount) + textureCount) % textureCount;
        assert(0 <= currentTextureIndex);
        assert(currentTextureIndex < textureCount);

        // Check if we have to change textures.
        if (currentTextureIndex != mCurrentTextureIndex) {
            String texture1 = mNoiseTextureNames[currentTextureIndex];
            String texture2 = mNoiseTextureNames[(currentTextureIndex + 1) % textureCount];
            //Ogre::LogManager::getSingleton ().logMessage (
            //        "Caelum: Switching cloud layer textures to " + texture1 + " and " + texture2);
            Ogre::Pass* pass = mMaterial->getBestTechnique()->getPass(0);
            pass->getTextureUnitState(0)->setTextureName(texture1);
            pass->getTextureUnitState(1)->setTextureName(texture2);
            mCurrentTextureIndex = currentTextureIndex;
        }

        Ogre::Real cloudMassBlend = fmod(mCloudBlendPos, 1);
		mParams.cloudMassBlend.set(mParams.fpParams, cloudMassBlend);
    }

    Ogre::Real FlatCloudLayer::getCloudBlendPos () const {
        return mCloudBlendPos;
    }

	void FlatCloudLayer::setCloudSpeed (const Ogre::Vector2 &cloudSpeed) {
		mCloudSpeed = cloudSpeed;
	}

	void FlatCloudLayer::setSunDirection (const Ogre::Vector3 &sunDirection) {
        mSunDirection = sunDirection;
		mParams.vpSunDirection.set(mParams.vpParams, sunDirection);
		mParams.fpSunDirection.set(mParams.fpParams, sunDirection);
	}

	void FlatCloudLayer::setSunLightColour (const Ogre::ColourValue &sunLightColour) {
		mParams.sunLightColour.set(mParams.fpParams, mSunLightColour = sunLightColour);
	}

	void FlatCloudLayer::setSunSphereColour (const Ogre::ColourValue &sunSphereColour) {
		mParams.sunSphereColour.set(mParams.fpParams, mSunSphereColour = sunSphereColour);
	}

	void FlatCloudLayer::setFogColour (const Ogre::ColourValue &fogColour) {
		mParams.fogColour.set(mParams.fpParams, mFogColour = fogColour);
	}

	const Ogre::Vector3 FlatCloudLayer::getSunDirection () const {
		return mSunDirection;
	}

	const Ogre::ColourValue FlatCloudLayer::getSunLightColour () const {
		return mSunLightColour;
	}

	const Ogre::ColourValue FlatCloudLayer::getSunSphereColour () const {
		return mSunSphereColour;
	}

    const Ogre::ColourValue FlatCloudLayer::getFogColour () const {
        return mFogColour;
    }

	void FlatCloudLayer::setHeight(Ogre::Real height) {
		mNode->setPosition(Ogre::Vector3(0, height, 0));
		mHeight = height;
		mParams.layerHeight.set(mParams.fpParams, mHeight);
	}

    Ogre::Real FlatCloudLayer::getHeight() const {
		return mHeight;
	}

    void FlatCloudLayer::setCloudUVFactor (const Ogre::Real value) {
		mParams.cloudUVFactor.set(mParams.fpParams, mCloudUVFactor = value);
    }

    void FlatCloudLayer::setHeightRedFactor (const Ogre::Real value) {
		mParams.heightRedFactor.set(mParams.fpParams, mHeightRedFactor = value);
    }

    void FlatCloudLayer::setFadeDistances (const Ogre::Real nearValue, const Ogre::Real farValue) {
        setNearFadeDist (nearValue);
        setFarFadeDist (farValue);
    }

    void FlatCloudLayer::setNearFadeDist (const Ogre::Real value) {
		mParams.nearFadeDist.set(mParams.fpParams, mNearFadeDist = value);
    }

    void FlatCloudLayer::setFarFadeDist (const Ogre::Real value) {
		mParams.farFadeDist.set(mParams.fpParams, mFarFadeDist = value);
    }

    void FlatCloudLayer::setFadeDistMeasurementVector (const Ogre::Vector3& value) {
		mParams.fadeDistMeasurementVector.set(mParams.fpParams, mFadeDistMeasurementVector = value);
    }
}
