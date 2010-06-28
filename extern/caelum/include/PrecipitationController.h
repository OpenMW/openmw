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

#ifndef CAELUM__PRECIPITATION_CONTROLLER_H
#define CAELUM__PRECIPITATION_CONTROLLER_H

#include "CaelumPrerequisites.h"
#include "FastGpuParamRef.h"

namespace Caelum
{
    /** Preset parameters for a certain type of precipitation.
     */
	struct PrecipitationPresetParams
	{
		Ogre::ColourValue Colour;
        Ogre::Real Speed;
		Ogre::String Name;
	};

    /** An enumeration of the available precipitation presets.
     *  @see PrecipitationController::getPrecipitationPreset
     */
	enum PrecipitationType
	{
		PRECTYPE_DRIZZLE		= 0,
		PRECTYPE_RAIN			= 1,
		PRECTYPE_SNOW			= 2,
		PRECTYPE_SNOWGRAINS	    = 3,
		PRECTYPE_ICECRYSTALS	= 4,
		PRECTYPE_ICEPELLETS	    = 5,
		PRECTYPE_HAIL			= 6,
		PRECTYPE_SMALLHAIL		= 7,

		PRECTYPE_CUSTOM		    = 8,
	};

    /** Compositor-based precipitation controller.
     *  This class will add and control precipitation controllers to viewports.
     *
     *  Compositors clone the composing materials. This controller will
     *  register itself as a compositor listener and change the material in notifyMaterialSetup.
     */
	class CAELUM_EXPORT PrecipitationController
	{
	private:
        friend class PrecipitationInstance;

		Ogre::SceneManager *mSceneMgr;
		Ogre::Vector3 mWindSpeed;
		Ogre::Real mIntensity;
		Ogre::Real mSpeed;
		Ogre::ColourValue mColour;
		PrecipitationType mPresetType;
		Ogre::String mTextureName;
        Ogre::Vector3 mCameraSpeedScale;
        Ogre::Vector3 mFallingDirection;

        Ogre::Real mAutoDisableThreshold;
        bool mHardDisableCompositor;

		Ogre::ColourValue mSceneColour;
		Real mInternalTime;

        // Only meant for the instance ctl in auto-camera-speed mode.
        Real mSecondsSinceLastFrame;
        inline Real getSecondsSinceLastFrame() { return mSecondsSinceLastFrame; }

	public:
        /// Name of the compositor resource.
        static const String COMPOSITOR_NAME;

        /// Name of the compositor material.
        static const String MATERIAL_NAME;

        /// Check if a preset type is valid.
		static bool isPresetType (PrecipitationType value);

        /// Get preset parameters for a certain type of precipitation.
		static const PrecipitationPresetParams& getPresetParams (PrecipitationType value);

        /// Set all parameters at once.
		void setParams (const PrecipitationPresetParams& params);

		/// Quickly set a certain preset type of precipitation.
        void setPresetType (PrecipitationType value);

        /** Get the preset type.
         *  Will return PRECIPITATION_CUSTOM if you modify parameters manually
         *  after setPresetType.
         */
        PrecipitationType getPresetType () const;

		// Texture name, part of a preset
		void setTextureName(const Ogre::String& textureName);
		const Ogre::String getTextureName() const;

		/// Precipitation color. Part of a preset
		void setColour(const Ogre::ColourValue& color);
		const Ogre::ColourValue getColour() const;

		/// Precipitation speed (affects direction). Part of a preset
		void setSpeed(Real value);
		Real getSpeed() const;

		/// Precipitation intensity.
		void setIntensity(Real value);
		Real getIntensity() const;

		/// Wind speed and direction
		void setWindSpeed(const Ogre::Vector3 &value);
		const Ogre::Vector3 getWindSpeed() const;

        /** The basic direction for falling precipitation.
         *
         *  This property define the notion of a "falling down" direction.
         *  By default this is Vector3::NEGATIVE_UNIT_Y. You need to change
         *  this for a Z-up system.
         */
        void setFallingDirection(const Ogre::Vector3 &value) { mFallingDirection = value; }
		const Ogre::Vector3 getFallingDirection() const { return mFallingDirection; }

		/// Set manual camera speed for all viewports.
		void setManualCameraSpeed(const Ogre::Vector3 &value);

        /// Set auto camera speed everywhere.o
		void setAutoCameraSpeed();

        /** Automatically disable compositors when intensity is low.
         *  A negative value always enable the compositor.
         *  @note: Ogre::CompositorInstance allocates/frees resources when
         *         enabling or disabling. That is expensive.
         */
        inline void setAutoDisableThreshold (Real value) { mAutoDisableThreshold = value; }
        inline Real getAutoDisableThreshold () const { return mAutoDisableThreshold; }

        /** Automatically scale camera speed.
         *
         *  This is multiplied per-component with camera speed; manual or
         *  automatic. It's most useful for automatic camera speed to control 
         *  how much of an effect moving the camera has on rain drop directions.
         *
         *  The components should probably be equal.
         *
         *  Default in Ogre::Vector3::UNIT_SCALE
         */
        inline void setCameraSpeedScale (const Ogre::Vector3& value) {
            mCameraSpeedScale = value;
        }
        inline const Ogre::Vector3 getCameraSpeedScale () const {
            return mCameraSpeedScale;
        }

        /** Set an equal camera speed scale in all dimensions.
         */
        inline void setCameraSpeedScale (Ogre::Real value) {
            setCameraSpeedScale(Ogre::Vector3::UNIT_SCALE * value);
        }

        /** Update the the precipitation controller.
         *  @param secondsSinceLastFrame Number of secods since the last frame.
         */
		void update(Real secondsSinceLastFrame, Ogre::ColourValue colour);

		PrecipitationController(
				Ogre::SceneManager *sceneMgr);
		~PrecipitationController();

    public:
        typedef std::map<Ogre::Viewport*, PrecipitationInstance*> ViewportInstanceMap;
        ViewportInstanceMap mViewportInstanceMap;

    public:
        /// Add precipitation to a certain viewport.
        PrecipitationInstance* createViewportInstance(Ogre::Viewport* viewport);

        /// Remove precipitation from a certain viewport.
        void destroyViewportInstance(Ogre::Viewport* viewport);

        /// Get per-viewport instance, or null if not created yet.
        PrecipitationInstance* getViewportInstance(Ogre::Viewport* viewport);

        /// Remove from all attached viewports; clean up.
        void destroyAllViewportInstances();
	};

    /** Per-viewport instance of precipitation.
     *  This will create and control an ogre::CompositorInstance.
     */
    class PrecipitationInstance: private Ogre::CompositorInstance::Listener
    {
    private:
        friend class PrecipitationController;

        PrecipitationController* mParent;
        Ogre::Viewport* mViewport;
        Ogre::CompositorInstance* mCompInst;
        Ogre::Camera* mLastCamera;
        Ogre::Vector3 mLastCameraPosition;
        Ogre::Vector3 mCameraSpeed;
		bool mAutoCameraSpeed;

        virtual void notifyMaterialSetup(uint pass_id, Ogre::MaterialPtr &mat);
        virtual void notifyMaterialRender(uint pass_id, Ogre::MaterialPtr &mat);

        /// Called to enforce parameters on a composing material
        /// Called from notifyMaterialRender.
    	void _updateMaterialParams(
                const Ogre::MaterialPtr& mat,
                const Ogre::Camera* cam,
                const Ogre::Vector3& camSpeed);

        // Add remove the compositors. Do nothing if already added/removed
        void createCompositor ();
        void destroyCompositor ();

        // Check if the compositor should be enabled
        bool shouldBeEnabled () const;

        /// Called from the parent's update.
        void _update();

    public:
        inline Ogre::Viewport* getViewport() const { return mViewport; }
        inline PrecipitationController* getParent() const { return mParent; }
        inline Ogre::CompositorInstance* getCompositorInstance() const { return mCompInst; }

        /// Check if camera speed is automatically calculated (default true).
        bool getAutoCameraSpeed();

        /** Set camera speed to automatic calculation.
         *
         *  @warning: This runs into difficult precission issues. It is
         *  better to use setManualCameraSpeed.
         */
        void setAutoCameraSpeed();

        /// Set manual camera speed; disables automatic calculation.
        void setManualCameraSpeed(const Ogre::Vector3& value);

        /// Get current camera speed. Doesn't include CameraSpeedScale.
        const Ogre::Vector3 getCameraSpeed();

        PrecipitationInstance(PrecipitationController* parent, Ogre::Viewport* view);
        virtual ~PrecipitationInstance();

    private:
        struct Params {
            void setup(Ogre::GpuProgramParametersSharedPtr fpParams);

            Ogre::GpuProgramParametersSharedPtr fpParams;
            FastGpuParamRef precColor;
            FastGpuParamRef intensity;
            FastGpuParamRef dropSpeed;
            FastGpuParamRef corner1;
            FastGpuParamRef corner2;
            FastGpuParamRef corner3;
            FastGpuParamRef corner4;
            FastGpuParamRef deltaX;
            FastGpuParamRef deltaY;
        } mParams;

    };
}

#endif //CAELUM__PRECIPITATION_CONTROLLER_H
