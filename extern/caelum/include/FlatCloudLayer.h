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

#ifndef CAELUM__FLAT_CLOUD_LAYER_H
#define CAELUM__FLAT_CLOUD_LAYER_H

#include "CaelumPrerequisites.h"
#include "InternalUtilities.h"
#include "PrivatePtr.h"
#include "FastGpuParamRef.h"

namespace Caelum
{
    /** A flat cloud layer; drawn as a simple plane.
     *  Supports movement and variable cloud cover.
     *  
     *  There are significant incompatible difference between this and the
     *  LayeredClouds from version 0.3. This implementation of clouds is
     *  positioned in world space while the old implementation was a curved
     *  plane moving with the camera. It is not possible to perfectly simulate
     *  the older implementation.
     *
     *  @note This is tighly integrated with LayeredCloud.cg and LayeredClouds.material.
     */
	class CAELUM_EXPORT FlatCloudLayer
	{
	public:
		FlatCloudLayer(
				Ogre::SceneManager *sceneMgr,
                Ogre::SceneNode *cloudRoot);

		~FlatCloudLayer();

        /** Update function called each frame from above.
         *  This can be reproduced with calls to other public functions.
         */
	    void update (
                Ogre::Real timePassed,
		        const Ogre::Vector3 &sunDirection,
		        const Ogre::ColourValue &sunLightColour,
		        const Ogre::ColourValue &fogColour,
				const Ogre::ColourValue &sunSphereColour);

        /// Advance cloud animation (the time part of FlatCloudLayer::update).
        void advanceAnimation (Ogre::Real timePassed);

        /** Reset most tweak settings to their default values
         */
        void reset ();

	private:
	    Ogre::Vector2 mCloudSpeed;
	    Ogre::Vector2 mCloudMassOffset;
	    Ogre::Vector2 mCloudDetailOffset;

        // Set texture offsets.
        // Animated every frame.
	    void setCloudMassOffset(const Ogre::Vector2 &cloudMassOffset);
	    void setCloudDetailOffset(const Ogre::Vector2 &cloudDetailOffset);

    public:
        /** Sets cloud movement speed.
		 *  @param cloudSpeed Cloud movement speed.
		 */
		void setCloudSpeed (const Ogre::Vector2 &cloudSpeed);

		/** Gets cloud movement speed.
		 *  @param cloudSpeed Cloud movement speed.
		 */
        const Ogre::Vector2 getCloudSpeed () const { return mCloudSpeed; }

    private:
        Ogre::Vector3 mSunDirection; 
        Ogre::ColourValue mSunLightColour; 
        Ogre::ColourValue mSunSphereColour; 
        Ogre::ColourValue mFogColour; 

    public:
	    void setSunDirection(const Ogre::Vector3 &sunDirection);
	    void setSunLightColour(const Ogre::ColourValue &sunLightColour);
		void setSunSphereColour(const Ogre::ColourValue &sunSphereColour);
	    void setFogColour(const Ogre::ColourValue &fogColour);
        const Ogre::Vector3 getSunDirection () const; 
        const Ogre::ColourValue getSunLightColour () const; 
        const Ogre::ColourValue getSunSphereColour () const; 
        const Ogre::ColourValue getFogColour () const; 

    private:
        /// Pointer to scene manager.
	    Ogre::SceneManager *mSceneMgr;

        // Note: objects are destroyed in reverse order of declaration.
        // This means that objects must be ordered by dependency.

        /// Cloned cloud material.
	    PrivateMaterialPtr mMaterial;		

        struct Params
        {
            void setup(Ogre::GpuProgramParametersSharedPtr fpParams, Ogre::GpuProgramParametersSharedPtr vpParams);

            Ogre::GpuProgramParametersSharedPtr vpParams;
            Ogre::GpuProgramParametersSharedPtr fpParams;

            FastGpuParamRef cloudCoverageThreshold;
            FastGpuParamRef cloudMassOffset;
            FastGpuParamRef cloudDetailOffset;
            FastGpuParamRef cloudMassBlend;
            FastGpuParamRef vpSunDirection;
            FastGpuParamRef fpSunDirection;
            FastGpuParamRef sunLightColour;
            FastGpuParamRef sunSphereColour;
            FastGpuParamRef fogColour;
            FastGpuParamRef layerHeight;
            FastGpuParamRef cloudUVFactor;
            FastGpuParamRef heightRedFactor;
            FastGpuParamRef nearFadeDist;
            FastGpuParamRef farFadeDist;
            FastGpuParamRef fadeDistMeasurementVector;
        } mParams;

    private:
        PrivateMeshPtr mMesh;
	    PrivateSceneNodePtr mNode;
	    PrivateEntityPtr mEntity;

        // Mesh parameters.
        bool mMeshDirty;
        Real mMeshWidth, mMeshHeight;
        int mMeshWidthSegments, mMeshHeightSegments;

    public:
        /** Regenerate the plane mesh and recreate entity.
         *  This automatically happens in update.
         */
        void _ensureGeometry();

        /** Regenerate the plane mesh and recreate entity.
         *  This automatically happens when mesh parameters are changed.
         */
        void _invalidateGeometry();

        /** Reset all mesh parameters.
         */
        void setMeshParameters (
                Real meshWidth, Real meshHeight,
                int meshWidthSegments, int meshHeightSegments);

        /// @see setMeshParameters
        inline void setMeshWidth (Real value) { mMeshWidth = value; _invalidateGeometry (); }
        inline void setMeshHeight (Real value) { mMeshHeight = value; _invalidateGeometry (); }
        inline void setMeshWidthSegments (int value) { mMeshWidthSegments = value; _invalidateGeometry (); }
        inline void setMeshHeightSegments (int value) { mMeshHeightSegments = value; _invalidateGeometry (); }
        inline Real getMeshWidth () const { return mMeshWidth; }
        inline Real getMeshHeight () const { return mMeshHeight; }
        inline int getMeshWidthSegments () const { return mMeshWidthSegments; }
        inline int getMeshHeightSegments () const { return mMeshHeightSegments; }

    private:
        /// Lookup used for cloud coverage, @see setCloudCoverLookup.
        std::auto_ptr<Ogre::Image> mCloudCoverLookup;

        /// Filename of mCloudCoverLookup
        Ogre::String mCloudCoverLookupFileName;

        /// Value passed to setCloudCover (before lookup).
	    Ogre::Real mCloudCover;

    public:
		/** Sets cloud cover, between 0 (completely clear) and 1 (completely covered)
		 *  @param cloudCover Cloud cover between 0 and 1
		 */
		void setCloudCover (const Ogre::Real cloudCover);

		/** Gets the current cloud cover.
		 *  @return Cloud cover, between 0 and 1
		 */
        inline Ogre::Real getCloudCover () const { return mCloudCover; }

        /** Set the image used to lookup the cloud coverage threshold.
         *  This image is used to calculate the cloud coverage threshold
         *  based on the desired cloud cover.
         *
         *  The cloud coverage threshold is substracted from cloud intensity
         *  at any point; to generate fewer or more clouds. That threshold is
         *  not linear, a lookup is required to ensure that setCloudCover(0.1)
         *  will actually have 10% the clouds at setCloudCover(1).
         *
         *  The lookup is the inverse of the sum on the histogram, and was
         *  calculated with a small hacky tool.
         */
	    void setCloudCoverLookup (const Ogre::String& fileName);

        /** Get the filename of the cloud cover lookup image.
         *  This returns the value set by setCloudCoverLookup or an empty
         *  string if disabled.
         */
        const Ogre::String getCloudCoverLookupFileName () const;

        /** Disable any cloud cover lookup.
         *  @see setCloudCoverLookup.
         */
        void disableCloudCoverLookup ();

    private:
        Ogre::Real mCloudCoverVisibilityThreshold;

    protected:
        /** Enforce setCloudCoverVisibilityThreshold.
         */
        void _updateVisibilityThreshold ();

    public:
        /// Get cloud cover visiblity threshold.
        /// Beneath this cloud coverage nothing is drawn anymore.
        Ogre::Real getCloudCoverVisibilityThreshold () const { return mCloudCoverVisibilityThreshold; }

        /** Set cloud cover visiblity threshold.
         *
         *  Beneath this cloud coverage nothing is drawn anymore.
         *  Default value is very very low (0.001). All this does is save you from
         *  destroying/recreating layers when they're too thin to bother drawing.
         */
        void setCloudCoverVisibilityThreshold (const Ogre::Real value);

    private:
        /// Height of this cloud layer; equal to node's y position.
        Ogre::Real mHeight;

    public:
        /** Set the height of the cloud layer.
         *  @param height In world units above the cloud root node.
         */
        void setHeight(Ogre::Real height);

        /** Get the height of the cloud layer.
         *  @return height In world units above the cloud root node.
         */
        Ogre::Real getHeight() const;

    private:
		/// Current cloud blend position; from 0 to mNoiseTextureNames.size()
	    Ogre::Real mCloudBlendPos;

        /// Current index in the set of textures.
        /// Cached to avoid setting textures every frame.
        int mCurrentTextureIndex;

        /// Time required to blend two cloud shapes.
	    Ogre::Real mCloudBlendTime;

        /// Names of noise textures.
        std::vector<String> mNoiseTextureNames;

    public:
	    /** Sets the time it takes to blend two cloud shaped together, in seconds.
         *  This will also reset the animation at the current time.
		 *  @param value Cloud shape blend time in seconds
		 */
		void setCloudBlendTime (const Ogre::Real value);

		/** Gets the time it takes to blend two cloud shaped together, in seconds.
		 *  @return Cloud shape blend time in seconds
		 */
		Ogre::Real getCloudBlendTime () const;

        /** Set the current blending position; between noise textures.
         *  Integer values are used for single textures. Float values blend between two textures.
         *  Values outside [0, textureCount) are wrapped around.
         *  @param value New cloud blending position
         */
	    void setCloudBlendPos (const Ogre::Real value);

        /// @see setCloudBlendPos
        Ogre::Real getCloudBlendPos () const;

    private:
        Ogre::Real mCloudUVFactor;

    public:
        /** Cloud texture coordinates are multiplied with this.
         *  Higher values result in more spread-out clouds.
         *  Very low value result in ugly texture repeats.
         */
	    void setCloudUVFactor (const Ogre::Real value);
        /// @see setCloudUVFactor
        inline Ogre::Real getCloudUVFactor () const { return mCloudUVFactor; }

    private:
        Ogre::Real mHeightRedFactor;

    public:
        /** High-altitude clouds are tinted red in the evening.
         *  Higher values attenuate the effect.
         */
	    void setHeightRedFactor (const Ogre::Real value);
        /// @see setCloudUVFactor
        Ogre::Real getHeightRedFactor () const { return mHeightRedFactor; }

    private:
        Ogre::Real mNearFadeDist;
        Ogre::Real mFarFadeDist;
        Ogre::Vector3 mFadeDistMeasurementVector;

    public:
        /** Cloud fade distances.
         *
         *  These are measured horizontally in meters (height is not used).
         *
         *  The effect is a fade based on alpha blending which occurs between
         *  nearValue and farValue. After farValue nothing is visibile from
         *  this layer.
         *
         *  Default values are 10000 and 140000
         */
        void setFadeDistances (Ogre::Real nearValue, Ogre::Real farValue);

        /// Set only near fade distance (see setFadeDistances).
	    void setNearFadeDist (const Ogre::Real value);
        /// Get near fade distance (see setFadeDistances).
        Ogre::Real getNearFadeDist () const { return mNearFadeDist; }

        /// Set only far fade distance (see setFadeDistances).
	    void setFarFadeDist (const Ogre::Real value);
        /// Get fade distance (see setFadeDistances).
        Ogre::Real getFarFadeDist () const { return mFarFadeDist; }

        /** Set on which components is the fade distance measured.
         *  
         *  Default is Vector3(0, 1, 1) which measures fade distance
         *  horizontally in caelum's default assumed coordinate system.
         *
         *  If you're in a Z-up system you probably want to set this to (1, 1, 0).
         *
         *  Fade distance is always measured relative to the camera.
         */
        void setFadeDistMeasurementVector (const Ogre::Vector3& value);
        /// Get the value set by setFadeDistMeasurementVector.
        const Ogre::Vector3 getFadeDistMeasurementVector () const { return mFadeDistMeasurementVector; }

    public:
        void setQueryFlags (uint flags) { mEntity->setQueryFlags (flags); }
        uint getQueryFlags () const { return mEntity->getQueryFlags (); }
        void setVisibilityFlags (uint flags) { mEntity->setVisibilityFlags (flags); }
        uint getVisibilityFlags () const { return mEntity->getVisibilityFlags (); }
	};
}

#endif // CAELUM__FLAT_CLOUD_LAYER_H
