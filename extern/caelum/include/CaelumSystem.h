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

#ifndef CAELUM__CAELUM_SYSTEM_H
#define CAELUM__CAELUM_SYSTEM_H

#include "CaelumPrerequisites.h"
#include "UniversalClock.h"
#include "ImageStarfield.h"
#include "PointStarfield.h"
#include "SkyLight.h"
#include "Sun.h"
#include "Moon.h"
#include "CloudSystem.h"
#include "SkyDome.h"
#include "DepthComposer.h"
#include "PrecipitationController.h"
#include "GroundFog.h"
#include "PrivatePtr.h"

namespace Caelum
{
    /** This is the "root class" of caelum.
     *
     *  This class is created once for one SceneManager and will render the sky
     *  for that scene. CaelumSystem will be visible in all viewports on the
     *  scene and must be notified when those viewports are created and
     *  destroyed.
     *  
     *  @par Components
     *
     *  %Caelum is built from several classes for different sky elements (the sun,
     *  clouds, etc). Those classes know very little about each other and are 
     *  connected through this class. This class is responsible for tracking and
     *  updating sub-components.
     *
     *  This class "owns" all of the subcomponents, using std::auto_ptr members.
     *  When you call functions like setXxx(new Xxx()) this class takes
     *  ownership of the object's lifetime and will try to update it as
     *  appropriate. All components are optional; disable one component should
     *  never cause a crash. When something is broken disabling components one
     *  by one is a very good way to find the source of the problem.
     *
     *  The constructor can create a bunch of components with default settings
     *  for you; based on the CaelumSystem::CaelumComponent flags passed.
     *
     *  @par Updating
     *
     *  This class is responsible for updating subcomponents. There are two
     *  update functions which must be get called to keep CaelumSystem
     *  functioning properly. One is per-frame and the other is per-camera.
     *
     *  CaelumSystem::updateSubcomponents must be called once per frame to
     *  advance world time and tie components together. That function will
     *  set certain properties on the subcomponents making up CaelumSystem
     *  If you want to force some properties beyond what CaelumSystem does by
     *  default you can do that AFTER the call to updateSubcompoments. For
     *  example you can override the moon's phase by calling Moon::setPhase.
     *
     *  CaelumSystem::notifyCameraChanged must be called for each camera
     *  before rendering with that camera. All viewport tweaks and camera
     *  movement must be done BEFORE calling this function. This method will
     *  recenter Caelum's domes on the camera. Also, some subcomponents
     *  can actually depend on field-of-view and viewport resolution (like
     *  PointStarfield).
     *
     *  You can register CaelumSystem as an Ogre::FrameListener and
     *  updateSubcomponents will be automatically called inside Ogre's
     *  rendering loop (inside frameStarted). If you want more control you
     *  should call updateSubcomponents in your own main loop. That way you
     *  can avoid potential issues with the ordering of multiple FrameListeners.
     *
     *  You can register CaelumSystem as an Ogre::RenderTargetListener and
     *  notifyCameraChanged will be automatically called inside
     *  preViewportUpdate. That behaviour can be disabled with
     *  setAutoNotifyCameraChanged(false). It is recommended that you 
     *  call notifyCameraChanged manually before updating viewports.
     *
     *  RenderTargetListener::preViewportUpdate does not work as expected
     *  when compositors are involved (those inside Caelum or external).
     *  Compositors plug into preRenderTargetUpdate and render the scene to a
     *  texture BEFORE preViewportUpdate; this means that notifyCameraChanged
     *  will execute before the final compositor pass but after actual scene
     *  rendering.
     *
     *  If notifyCameraChanged is not called correctly the most likely result
     *  is "flickering" when moving the camera. If you move the camera AFTER
     *  notifyCameraChanged then the domes will not be positioned correctly
     *  and will appear to lag slightly after the camera. Since updates are
     *  always done every frame keeping the camera still will make problems
     *  disappear.
     *
     *  If you notice z-buffer issues while the camera is still update order
     *  is probably not the cause.
     */
    class CAELUM_EXPORT CaelumSystem:
            public Ogre::FrameListener,
            public Ogre::RenderTargetListener
    {
	private:
		/// Root of the Ogre engine.
		Ogre::Root *mOgreRoot;

		/// Scene manager.
		Ogre::SceneManager *mSceneMgr;

        /// Caelum scene node for camera-bound elements (most).
		PrivateSceneNodePtr mCaelumCameraNode;

        /// Caelum scene node for ground-bound elements (only clouds currently).
		PrivateSceneNodePtr mCaelumGroundNode;

		/// Cleanup requested flag.
		bool mCleanup;

        /// Automatically move the camera node.
        bool mAutoMoveCameraNode;

        /// Automatically call this->notifyCameraChanged.
        bool mAutoNotifyCameraChanged;

        /// Automatically attach compositors to viewports
        bool mAutoAttachViewportsToComponents;

        /// Automatically set the viewport colour to black.
        bool mAutoViewportBackground;

        /// Flag to indicate if Caelum manages standard Ogre::Scene fog.
		bool mManageSceneFog;

        Real mGlobalFogDensityMultiplier;
        Ogre::ColourValue mGlobalFogColourMultiplier;

        Real mSceneFogDensityMultiplier;
        Ogre::ColourValue mSceneFogColourMultiplier;

        Real mGroundFogDensityMultiplier;
        Ogre::ColourValue mGroundFogColourMultiplier;

        /// Flag for managing scene ambient light.
		bool mManageAmbientLight;

        /// Minimum ambient light; only useful if mManageAmbientLight
        Ogre::ColourValue mMinimumAmbientLight;

        /// If only one light source should enabled at a time.
        bool mEnsureSingleLightSource;

        /// Ensure only one of the light sources casts shadows.
        bool mEnsureSingleShadowSource;

		/// The sky gradients image (for lookups).
        std::auto_ptr<Ogre::Image> mSkyGradientsImage;

        /// The sun gradients image (for lookups).
        std::auto_ptr<Ogre::Image> mSunColoursImage;

        /// Observer Latitude (on the earth).
        Ogre::Degree mObserverLatitude;
        /// Observer Longitude (on the earth).
        Ogre::Degree mObserverLongitude;

        static const Ogre::Vector3 makeDirection (
                Ogre::Degree azimuth, Ogre::Degree altitude);
		
		// References to sub-components
        std::auto_ptr<UniversalClock> mUniversalClock;
        std::auto_ptr<SkyDome> mSkyDome;
        std::auto_ptr<BaseSkyLight> mSun;
        std::auto_ptr<Moon> mMoon;
        std::auto_ptr<ImageStarfield> mImageStarfield;
        std::auto_ptr<PointStarfield> mPointStarfield;
        std::auto_ptr<GroundFog> mGroundFog;
        std::auto_ptr<CloudSystem> mCloudSystem;
		std::auto_ptr<PrecipitationController> mPrecipitationController;
		std::auto_ptr<DepthComposer> mDepthComposer;

    public:
        typedef std::set<Ogre::Viewport*> AttachedViewportSet;

    private:
        AttachedViewportSet mAttachedViewports;

	public:
        /** Flags enumeration for caelum components.
         *  This is an enumeration for the components to create by default in
         *  Caelum's constructor. You can still pass 0 and create everything
         *  by hand.
         * 
         *  CaelumSystem's constructor used to take a number of bools but now
         *  there are too many components and this is nicer.
         * 
         *  CAELUM_COMPONENT_ members are for individual components.
         *  CAELUM_COMPONENTS_ are standard bitmasks.
         *  CAELUM_COMPONENTS_DEFAULT picks elements that don't require
         *  modifications to external materials (right now it excludes ground fog).
         */
        enum CaelumComponent
        {
            CAELUM_COMPONENT_SKY_DOME           = 1 << 1,
            CAELUM_COMPONENT_MOON				= 1 << 3,
            CAELUM_COMPONENT_SUN                = 1 << 4,
            CAELUM_COMPONENT_IMAGE_STARFIELD    = 1 << 5,
            CAELUM_COMPONENT_POINT_STARFIELD    = 1 << 6,
            CAELUM_COMPONENT_CLOUDS             = 1 << 7,
            CAELUM_COMPONENT_PRECIPITATION      = 1 << 8,
            CAELUM_COMPONENT_SCREEN_SPACE_FOG   = 1 << 9,

            // This has nasty dependencies on materials.
            CAELUM_COMPONENT_GROUND_FOG         = 1 << (16 + 0),

            // Groups
            CAELUM_COMPONENTS_NONE              = 0,
            CAELUM_COMPONENTS_DEFAULT           = 0
                    | CAELUM_COMPONENT_SKY_DOME
                    | CAELUM_COMPONENT_MOON
                    | CAELUM_COMPONENT_SUN
                    | CAELUM_COMPONENT_POINT_STARFIELD
                    | CAELUM_COMPONENT_CLOUDS,
            CAELUM_COMPONENTS_ALL               = 0
                    | CAELUM_COMPONENTS_DEFAULT
                    | CAELUM_COMPONENT_PRECIPITATION 
                    | CAELUM_COMPONENT_SCREEN_SPACE_FOG
                    | CAELUM_COMPONENT_GROUND_FOG,
        };

        static const String DEFAULT_SKY_GRADIENTS_IMAGE;
        static const String DEFAULT_SUN_COLOURS_IMAGE;
    
		/** Constructor.
		 *  Registers itself in the Ogre engine and initialises the system.
         *
         *  @param root The Ogre root.
		 *  @param scene The Ogre scene manager.
		 *  @param componentsToCreate Default components for @see autoConfigure.
		 */
		CaelumSystem (
                Ogre::Root *root, 
				Ogre::SceneManager *sceneMgr,
                CaelumComponent componentsToCreate);

        /** Revert everything to defaults.
         *
         *  This function will delete all subcomponents and revert everything
         *  to default values (the values which are also set on construction).
         */
        void clear ();

        /** Create some default component with resonable default settings.
         *  This results in a slightly cloudy morning sky.
         *  This will always call clear() before creating components.
         *  autoConfigure (0); is equivalent to clear();
         */
        void autoConfigure (
                CaelumComponent componentsToCreate);

		/** Destructor.
		 */
		~CaelumSystem ();

		/** Shuts down the system and detaches itself from the Ogre engine.
         *
         *  shutdown(true) is equivalent to deleting CaelumSystem yourself.
         *  shutdown(false) delays destruction to the next time caelum is called as
         *  a frame listener. This makes it safe to shutdown Caelum from inside
         *  another frame listener.
         *
         *  @param cleanup If this is true then detach and destroy the CaelumSystem instantly.
		 */
		void shutdown (bool cleanup);

        /** Update the whole system manually.
         *  You have to call this yourself if you don't register CaelumSystem
         *  as an ogre frame listener. Otherwise it's called automatically.
         *
         *  @param timeSinceLastFrame: Time passed since last frame.
         */
        void updateSubcomponents (Real timeSinceLastFrame);

        /** Notify subcomponents of camera changes.
         *  This function must be called after camera changes but before
         *  rendering with that camera. If multiple cameras are used it must
         *  be called for each camera before the camera is rendered with.
         *
         *  This function will move caelum's camera node to the camera
         *  position, but only if getAutoMoveCameraNode.
         *  It will also call CameraBoundElement::notifyCameraChanged
         */
        void notifyCameraChanged(Ogre::Camera* cam);

        /** Get the scene manager for this caelum system.
         *  This is set in the constructor. CaelumSystem can't exist without a valid scene manager.
         */
        inline Ogre::SceneManager* getSceneMgr() const { return mSceneMgr; }

		/// Gets root scene node for camera-bound elements
        inline Ogre::SceneNode* getCaelumCameraNode(void) const { return mCaelumCameraNode.get(); }

		/// Gets root scene node for ground-bound elements.
        inline Ogre::SceneNode* getCaelumGroundNode(void) const { return mCaelumGroundNode.get(); }

        /** If true; listen to preViewportUpdate and automatically notifyCameraChanged();
         *
         *  This is on by default; but does not work with compositors.
         *
         *  You must attach CaelumSystem as a RenderTargetListener manually for
         *  this to work; as in version 0.3.
         */
        inline void setAutoNotifyCameraChanged(bool value) { mAutoNotifyCameraChanged = value; }
        /// @see setAutoNotifyCameraChanged
        inline bool getAutoNotifyCameraChanged() const { return mAutoNotifyCameraChanged; }

        /** If true; automatically attach viewports to subcomponents.
         *
         *  Some subcomponents use compositors and those compositors need to
         *  be attached to individual viewports. By default CaelumSystem will
         *  try take to take care of that automatically.
         *
         *  This property allows you to disable that behaviour. If set to false
         *  you must call functions like
         *  PrecipitationController::createViewportInstance manually.
         *
         *  @see attachViewport detachViewport
         */
        inline void setAutoAttachViewportsToComponents(bool value) { mAutoAttachViewportsToComponents = value; }
        /// @see setAutoAttachViewportsToComponents.
        inline bool getAutoAttachViewportsToComponents() const { return mAutoAttachViewportsToComponents; }

        /** If true (default); automatically move the camera node in notifyCameraChanged.
         *  If disable you get full control of the camera node; and in theory
         *  you can attach it to the scene graph however you please.
         */
        inline void setAutoMoveCameraNode(bool value) { mAutoMoveCameraNode = value; }
        /// @see setAutoMoveCameraNode
        inline bool getAutoMoveCameraNode() { return mAutoMoveCameraNode; }

        /** If true; automatically set the viewport color to black.
         *  Caelum's domes relies on the viewport background being black.
         *  There's generally no reason to disable this and it's on by default.
         */
        inline void setAutoViewportBackground(bool value) { mAutoViewportBackground = value; }
        /// @see setAutoViewportBackground
        inline bool getAutoViewportBackground() const { return mAutoViewportBackground; }

        /// Get the observer's longitude. East is positive, west is negative.
        inline const Ogre::Degree getObserverLongitude () const { return mObserverLongitude; }

        /// Set the observer's longitude. East is positive, west is negative.
        inline void setObserverLongitude (Ogre::Degree value) { mObserverLongitude = value; }

        /// Get the observer's latitude. North is positive, south is negative.
        inline const Ogre::Degree getObserverLatitude () const { return mObserverLatitude; }

        /// Set the observer's latitude. North is positive, south is negative.
        inline void setObserverLatitude (Ogre::Degree value) { mObserverLatitude = value; }

        inline LongReal getJulianDay () const { return mUniversalClock->getJulianDay (); }
        inline void setJulianDay (LongReal value) { mUniversalClock->setJulianDay (value); }
        inline Real getTimeScale () const { return mUniversalClock->getTimeScale (); }
        inline void setTimeScale (Real value) { mUniversalClock->setTimeScale (value); }

    public:
        /** Attach CaelumSystem to a viewport.
         *  You should call this for every new viewport looking at the scene
         *  where CaelumSystem is created.
         *
         *  If the viewport is already attached then nothing happens.
         *
         *  If getAutoAttachViewportsToComponents() this will add Caelum's compositors.
         */
		void attachViewport (Ogre::Viewport* rt);

        /** Reverse of @see attachViewport.
         *  You need to call this when you destroy a viewport.
         *
         *  If the viewport is not already attached nothing happens.
         */
		void detachViewport (Ogre::Viewport* rt);

        /** Check if one particular viewport is attached.
         */
        bool isViewportAttached (Ogre::Viewport* vp) const;

        /** Detach from all viewports.
         */
        void detachAllViewports ();

        /// Get a reference to the set of attached viewports.
        const AttachedViewportSet& _getAttachedViewportSet () { return mAttachedViewports; }

    protected:
        // Do the work behind attach/detach viewport.
		void attachViewportImpl (Ogre::Viewport* rt);
		void detachViewportImpl (Ogre::Viewport* rt);
        
    public:
		/// Gets the universal clock.
        inline UniversalClock *getUniversalClock () const { return mUniversalClock.get(); }

		/// Get the current sky dome, or null if disabled.
        inline SkyDome* getSkyDome () const { return mSkyDome.get (); }
		/// Set the skydome, or null to disable.
        void setSkyDome (SkyDome *obj);

		/// Gets the current sun, or null if disabled.
        inline BaseSkyLight* getSun () const { return mSun.get (); }
		/// Set the sun, or null to disable.
		void setSun (BaseSkyLight* obj);

		/// Gets the current moon, or null if disabled.
        inline Moon* getMoon () const { return mMoon.get (); }
		/// Set the moon, or null to disable.
		void setMoon (Moon* obj);

		/// Gets the current image starfield, or null if disabled.
        inline ImageStarfield* getImageStarfield () const { return mImageStarfield.get (); }
		/// Set image starfield, or null to disable.
        void setImageStarfield (ImageStarfield* obj);

		/// Gets the current point starfield, or null if disabled.
        inline PointStarfield* getPointStarfield () const { return mPointStarfield.get (); }
		/// Set image starfield, or null to disable.
        void setPointStarfield (PointStarfield* obj);

		/// Get ground fog; if enabled.
        inline GroundFog* getGroundFog () { return mGroundFog.get (); }
		/// Sets ground fog system, or null to disable.
        void setGroundFog (GroundFog *obj);

        /// Get cloud system; or null if disabled.
        inline CloudSystem* getCloudSystem () { return mCloudSystem.get (); }
        /// Set cloud system; or null to disable.
        void setCloudSystem (CloudSystem *obj);

        /// Get precipitation controller; or null if disabled.
		inline PrecipitationController* getPrecipitationController () { return mPrecipitationController.get (); }
        /// Set precipitation controller; or null to disable.
		void setPrecipitationController (PrecipitationController *obj);
 
        /// Get depth composer; or null if disabled.
		inline DepthComposer* getDepthComposer () { return mDepthComposer.get (); }
        /// Set depth composer; or null to disable.
		void setDepthComposer (DepthComposer *obj);
 
		/** Enables/disables Caelum managing standard Ogre::Scene fog.
            This makes CaelumSystem control standard Ogre::Scene fogging. It
            will use EXP2 fog with density from SkyColourModel.

            Fog density multipliers are used; final scene fog density is:
            SceneMultiplier * GlobalMultiplier * SkyColourModel.GetFogDensity

            When this is set to false it also disables all scene fog (but you
            control it afterwards).

            @param value New value
		 */
		void setManageSceneFog (bool value);

		/** Tells if Caelum is managing the fog or not.
			@return The value set in setManageSceneFog.
		 */
		bool getManageSceneFog () const;

        /** Multiplier for scene fog density (default 1).
            This is an additional multiplier for Ogre::Scene fog density.
            This has no effect if getManageSceneFog is false.

            Final scene fog density is:
            SceneMultiplier * GlobalMultiplier * SkyColourModel.GetFogDensity
         */
        void setSceneFogDensityMultiplier (Real value);

        /** Get the value set by setSceneFogDensityMultiplier.
         */
        Real getSceneFogDensityMultiplier () const;

        /** Set an additional multiplier for fog colour as it comes from SkyColourModel.
         *  This is 0.7 by default; to be compatible with previous versions.
         */
        inline void setSceneFogColourMultiplier (const Ogre::ColourValue& value) { mSceneFogColourMultiplier = value; }

        /// See setSceneFogColourMultiplier.
        inline const Ogre::ColourValue getSceneFogColourMultiplier () const { return mSceneFogColourMultiplier; }

        /** Multiplier for ground fog density (default 1).
         *  This is an additional multiplier for Caelum::GroundFog DepthComposer ground fog density.
         *
         *  Final ground fog density is:
         *  GroundFogMultipler * GlobalMultiplier * SkyColourModel.GetFogDensity
         */
        void setGroundFogDensityMultiplier (Real value);

        /** Get the value set by setGroundFogDensityMultiplier.
         */
        Real getGroundFogDensityMultiplier () const;

        /** Set an additional multiplier for ground fog colour as it comes from SkyColourModel.
         *  This is OgreColour::White by default; which has no effect.
         */
        inline void setGroundFogColourMultiplier (const Ogre::ColourValue& value) { mGroundFogColourMultiplier = value; }

        /// See setGroundFogColourMultiplier.
        inline const Ogre::ColourValue getGroundFogColourMultiplier () const { return mGroundFogColourMultiplier; }

        /** Multiplier for global fog density (default 1).
         *  This is an additional multiplier for fog density as received from
         *  SkyColourModel. There are other multipliers you can tweak for
         *  individual kinds of fog; but this is what you should change from
         *  whatever "game logic" you might have.
         */
        void setGlobalFogDensityMultiplier (Real value);

        /** Get the value set by setSceneFogDensityMultiplier.
         */
        Real getGlobalFogDensityMultiplier () const;

        /** Set an additional multiplier for fog colour.
         *  This will also affect stuff like clouds or precipitation. Careful!
         *  This is OgreColour::White by default; which has no effect.
         */
        inline void setGlobalFogColourMultiplier (const Ogre::ColourValue& value) { mGlobalFogColourMultiplier = value; }

        /// See setGlobalFogColourMultiplier.
        inline const Ogre::ColourValue getGlobalFogColourMultiplier () const { return mGlobalFogColourMultiplier; }

        /** Set this to true to have CaelumSystem manage the scene's ambient light.
         *  The colour and AmbientMultiplier of the sun and moon are used.
         *  This is false by default.
         */
        inline void setManageAmbientLight (bool value) { mManageAmbientLight = value; }

        /// Check if CaelumSystem is managing ambient lighting.
        inline bool getManageAmbientLight () const { return mManageAmbientLight; }

        /** Set the minimum value for scene ambient lighting,
         *  This is only used if getManageAmbientLight() is true.
         *  By default this value is Ogre::ColourValue::Black, so it has no effect.
         */
        inline void setMinimumAmbientLight (const Ogre::ColourValue &value) { mMinimumAmbientLight = value; }

        /// @see setMinimumAmbientLight
        inline const Ogre::ColourValue getMinimumAmbientLight () const { return mMinimumAmbientLight; }

        /** Ensure only one of caelum's light sources is active at a time (the brightest).
         *  This uses SkyLight::setForceDisable to disable low-intensity lightsources.
         *  Their contribution to ambient lighting is not affected.
         *  This implies a single shadow caster.
         *  This is disabled by default; and you can tweak light disabling by yourself.
         */
        inline void setEnsureSingleLightSource (bool value) { mEnsureSingleLightSource = value; }

        /// See setEnsureSingleLightSource
        inline bool getEnsureSingleLightSource () const { return mEnsureSingleLightSource; }

        /** Ensure only one of caelum's light sources casts shadows (the brightest).
         *  Disabled by default.
         */
        inline void setEnsureSingleShadowSource (bool value) { mEnsureSingleShadowSource = value; }

        /// See setEnsureSingleShadowSource
        inline bool getEnsureSingleShadowSource () const { return mEnsureSingleShadowSource; }

		/** Gets the fog colour for a certain daytime.
			@param time The current time.
			@param sunDir The sun direction.
			@return The fog colour.
		 */
		Ogre::ColourValue getFogColour (Real time, const Ogre::Vector3 &sunDir);

		/** Gets the fog density for a certain daytime.
			@param time The current time.
			@param sunDir The sun direction.
			@return The fog density.
		 */
		Real getFogDensity (Real time, const Ogre::Vector3 &sunDir);

		/** Get the colour of the sun sphere.
         *  This colour is used to draw the sun sphere in the sky.
		 *  @return The colour of the sun.
		 */
		Ogre::ColourValue getSunSphereColour (Real time, const Ogre::Vector3 &sunDir);

		/** Gets the colour of sun light.
         *  This color is used to illuminate the scene.
         *  @return The colour of the sun's light
		 */
		Ogre::ColourValue getSunLightColour (Real time, const Ogre::Vector3 &sunDir);

		/// Gets the colour of moon's body.
		Ogre::ColourValue getMoonBodyColour (const Ogre::Vector3 &moonDir);

		/// Gets the colour of moon's light.
		Ogre::ColourValue getMoonLightColour (const Ogre::Vector3 &moonDir);

		/// Set the sun gradients image.
		void setSkyGradientsImage (const Ogre::String &filename = DEFAULT_SKY_GRADIENTS_IMAGE);

		/// Set the sun colours image.
		/// Sun colour is taken from this image.
		void setSunColoursImage (const Ogre::String &filename = DEFAULT_SUN_COLOURS_IMAGE);

		/** Get the sun's direction at a certain time.
         *  @param jday astronomical julian day.
         *  @see UniversalClock for julian day calculations.
		 */
		const Ogre::Vector3 getSunDirection (LongReal jday);

		/** Get the moon's direction at a certain time.
         *  @param jday astronomical julian day.
		 */
		const Ogre::Vector3 getMoonDirection (LongReal jday);

        /** Fake function to get the phase of the moon
         *  @param jday Julian day
         *  @return the phase of the moon; ranging from 0(full moon) to 2(new moon).
         *  The calculations performed by this function are completely fake.
         *  It's a triangle wave with a period of 28.5 days.
         */
		const Ogre::Real getMoonPhase (LongReal jday);

    private:
		/** Handle FrameListener::frameStarted to call updateSubcomponents every frame.
         *  If you don't register CaelumSystem as a an ogre frame listener you have to
         *  call updateSubcomponents yourself.
		 */
		virtual bool frameStarted (const Ogre::FrameEvent &e);

		/** Event trigger called just before rendering a viewport in a render target Caelum is attached to.
			Useful to make objects follow every camera that renders a viewport in a certain render target.
		 */
		virtual void preViewportUpdate (const Ogre::RenderTargetViewportEvent &e);

        /** Free all subcomponents, but not CaelumSystem itself. Can be called multiple times.
         *  @param everything To destroy things that can't be rebuilt.
         */
        void destroySubcomponents (bool everything);

    public:
        /** Call setQueryFlags for all subcomponents now.
         *
         *  This is not persistent; you can adjust the query masks of
         *  individual objects afterwards. This also means you should call
         *  this only after you created all other objects.
         *
         *  Has no effect on compositor-based stuff (precipitation will still show up).
         */
        void forceSubcomponentQueryFlags (uint mask);

        /** Same as @see forceSubcomponentQueryMask; but for visibility
         */
        void forceSubcomponentVisibilityFlags (uint mask);
    };
}

#endif // CAELUM__CAELUM_SYSTEM_H
