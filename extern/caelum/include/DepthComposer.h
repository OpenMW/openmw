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

#ifndef CAELUM__DEPTH_COMPOSER_H
#define CAELUM__DEPTH_COMPOSER_H

#include "CaelumPrerequisites.h"
#include "FastGpuParamRef.h"

namespace Caelum
{
    /** Compositor-based precipitation controller.
     *  This class will add and control precipitation controllers to viewports.
     *
     *  Compositors clone the composing materials. This controller will
     *  register itself as a compositor listener and change the material in notifyMaterialSetup.
     */
	class CAELUM_EXPORT DepthComposer
	{
	private:
		Ogre::SceneManager *mSceneMgr;

        void onCompositorMaterialChanged ();
        const String& getCompositorName ();

    protected:
        inline Ogre::SceneManager* getSceneManager() const { return mSceneMgr; }

        friend class DepthComposerInstance;

    public:
		DepthComposer(Ogre::SceneManager *sceneMgr);
		virtual ~DepthComposer();

        void update ();

    public:
        typedef std::map<Ogre::Viewport*, DepthComposerInstance*> ViewportInstanceMap;
        ViewportInstanceMap mViewportInstanceMap;

    public:
        DepthComposerInstance* createViewportInstance(Ogre::Viewport* viewport);
        void destroyViewportInstance(Ogre::Viewport* viewport);
        DepthComposerInstance* getViewportInstance(Ogre::Viewport* viewport);
        void destroyAllViewportInstances();

    private:
        bool mDebugDepthRender;

    public:
		/// Enables drawing the depth buffer
		void setDebugDepthRender (bool value);
		bool getDebugDepthRender () const { return mDebugDepthRender; }

    private:
        bool mSkyDomeHazeEnabled;
        Ogre::Vector3 mSunDirection;
        Ogre::ColourValue mHazeColour;

    public:
		/// Enables skydome haze.
		void setSkyDomeHazeEnabled (bool value);
		bool getSkyDomeHazeEnabled () const { return mSkyDomeHazeEnabled; }

        void setSunDirection (const Ogre::Vector3& value) { mSunDirection = value; }
        const Ogre::Vector3 getSunDirection () const { return mSunDirection; }

        void setHazeColour (const Ogre::ColourValue& value) { mHazeColour = value; }
        const Ogre::ColourValue getHazeColour () const { return mHazeColour; }

    private:
        bool mGroundFogEnabled;
        Real mGroundFogDensity;
        Real mGroundFogBaseLevel;
        Real mGroundFogVerticalDecay;
        Ogre::ColourValue mGroundFogColour;

    public:
		/// Enables exponential ground fog.
		void setGroundFogEnabled (bool value);
		bool getGroundFogEnabled () const { return mGroundFogEnabled; }

		/// Sets ground fog density
		void setGroundFogDensity (Real value) { mGroundFogDensity = value; }

		/// Get ground fog density
		Real getGroundFogDensity () const { return mGroundFogDensity; }

		/// Sets ground fog level
        /// At ground level fogginess is equal to GroundFogDensity
		void setGroundFogBaseLevel (Real value) { mGroundFogBaseLevel = value; }

		/// Get ground fog density
		Real getGroundFogBaseLevel () const { return mGroundFogBaseLevel; }

		/// Sets ground fog vertical decay
		void setGroundFogVerticalDecay (Real value) { mGroundFogVerticalDecay = value; }

		/// Get ground fog density
		Real getGroundFogVerticalDecay () const { return mGroundFogVerticalDecay; }

		/// Sets ground fog colour
		void setGroundFogColour (const Ogre::ColourValue& value) { mGroundFogColour = value; }

		/// Get ground fog colour
		const Ogre::ColourValue getGroundFogColour () const { return mGroundFogColour; }
	};

    /** Per-viewport instance of @see DepthComposer
     *  This will create and control one ogre::CompositorInstance.
     */
    class CAELUM_EXPORT DepthComposerInstance: private Ogre::CompositorInstance::Listener
    {
    private:
        DepthComposer* mParent;
        Ogre::Viewport* mViewport;
        Ogre::CompositorInstance* mCompInst;
        std::auto_ptr<DepthRenderer> mDepthRenderer;

        virtual void notifyMaterialSetup(uint pass_id, Ogre::MaterialPtr &mat);
        virtual void notifyMaterialRender(uint pass_id, Ogre::MaterialPtr &mat);

        struct Params {
            void setup(Ogre::GpuProgramParametersSharedPtr params);

            Ogre::GpuProgramParametersSharedPtr fpParams;
            FastGpuParamRef invViewProjMatrix;
            FastGpuParamRef worldCameraPos;
            FastGpuParamRef groundFogDensity;
            FastGpuParamRef groundFogVerticalDecay;
            FastGpuParamRef groundFogBaseLevel;
            FastGpuParamRef groundFogColour;
            FastGpuParamRef sunDirection;
            FastGpuParamRef hazeColour;
        } mParams;

    protected:
        /// Called from DepthComposer::update
        void _update ();

        void addCompositor ();
        void removeCompositor ();
        bool isCompositorEnabled () { return mCompInst != 0; }

        friend class DepthComposer;

    public:
        /// Get parent DepthComposer; with all the interesting parameters.
        DepthComposer* getParent() const { return mParent; }

        /// Get the viewport this instance is attached to.
        Ogre::Viewport* getViewport() const { return mViewport; }

        /// Get compositor instance; attached to the viewport.
        Ogre::CompositorInstance* getCompositorInstance() const { return mCompInst; }

        /** Get the underlying depth renderer.
         *  Allow the user to tweak the depth renderer.
         */
        Caelum::DepthRenderer* getDepthRenderer () const { return mDepthRenderer.get(); }

        DepthComposerInstance(DepthComposer* parent, Ogre::Viewport* view);
        virtual ~DepthComposerInstance();
    };
    
    /** Render the depth buffer to a texture.
     *
     *  This class tries to be as generic and flexible as possible; but it 
     *  is currently only used by the depth composer.
     */
    class CAELUM_EXPORT DepthRenderer: private Ogre::RenderQueue::RenderableListener
    {
    private:
        Ogre::Viewport* mMasterViewport;
        Ogre::Viewport* mDepthRenderViewport;
        Ogre::TexturePtr mDepthRenderTexture;
        bool mDepthRenderingNow;
		Ogre::MaterialPtr mDepthRenderMaterial;

        // Override materials during all rendering.
#if OGRE_VERSION < 0x00010600
        virtual bool renderableQueued(
                Ogre::Renderable* rend,
                Ogre::uint8 groupId,
                Ogre::ushort priority, 
                Ogre::Technique** ppTech);
#else
        virtual bool renderableQueued(
                Ogre::Renderable* rend,
                Ogre::uint8 groupId,
                Ogre::ushort priority, 
                Ogre::Technique** ppTech,
                Ogre::RenderQueue* pQueue);
#endif // OGRE_VERSION
        inline Ogre::Material* getDepthRenderMaterial() const { return mDepthRenderMaterial.get(); }

        int mMinRenderGroupId;
        int mMaxRenderGroupId;
        int mViewportVisibilityMask;

    public:
        DepthRenderer (Ogre::Viewport* viewport);
        ~DepthRenderer ();

        inline Ogre::Viewport* getMasterViewport() { return mMasterViewport; }
        inline Ogre::Texture* getDepthRenderTexture () { return mDepthRenderTexture.get(); }
        inline Ogre::Viewport* getDepthRenderViewport () { return mDepthRenderViewport; }
        inline Ogre::RenderTexture* getDepthRenderTarget () {
            return mDepthRenderTexture->getBuffer()->getRenderTarget ();
        }

        /// Render the depth buffer now!
        void update ();

        /** Render only the render groups in a certain range.
         *  Call this to only render objects in certain render queue groups.
         *  The range is inclusive.
         *  This is a very primitive sort of filter.
         */
        void setRenderGroupRangeFilter (int minGroup, int maxGroup);

        /// @see setRenderGroupRangeFilter
        int getRenderGroupRangeFilterMin () { return mMinRenderGroupId; }
        int getRenderGroupRangeFilterMax () { return mMaxRenderGroupId; }

        /** Disable the effects of @see setRenderGroupRangeFilter
         */
        void disableRenderGroupRangeFilter ();

        /** Query mask for the depth rendering viewport.
         *  Enforces on every update ();
         */
        void setViewportVisibilityMask (uint value) { mViewportVisibilityMask = value; }
        uint getViewportVisibilityMask () { return mViewportVisibilityMask; }
        void disableViewportVisibilityMask () { mViewportVisibilityMask = ~0; }

    public:
        /** If true then use a user-supplied material scheme which outputs depth.
         *
         *  The depth output of most materials is obvious and can be guessed most of the time.
         *  When that fails you can provide a custom material scheme for certain materials which
         *  renders the depth buffer.
         *
         *  This is enabled by default for a scheme called CaelumDepth.
         */
        inline void setUseCustomDepthScheme (bool value) { mUseCustomDepthScheme = value; }
        inline bool getUseCustomDepthScheme () { return mUseCustomDepthScheme; }

        /** Set the name of the custom depth scheme (default is CaelumDepth).
         */
        inline void setCustomDepthSchemeName (const Ogre::String& value) { mCustomDepthSchemeName = value; }
        inline const Ogre::String& getCustomDepthSchemeName () { return mCustomDepthSchemeName; }

        /// Default name of the custom scheme.
        static const String DEFAULT_CUSTOM_DEPTH_SCHEME_NAME;

    private:
        bool mUseCustomDepthScheme;
        Ogre::String mCustomDepthSchemeName;
    };
}

#endif // CAELUM__DEPTH_COMPOSER_H
