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
#include "CaelumExceptions.h"
#include "DepthComposer.h"

using namespace Ogre;

namespace Caelum
{
	DepthComposer::DepthComposer
    (
        Ogre::SceneManager *sceneMgr
    ):
		mSceneMgr (sceneMgr),
        mDebugDepthRender (false),
        mSkyDomeHazeEnabled (false),
        mGroundFogEnabled (false),
        mGroundFogDensity (0.1),
        mGroundFogBaseLevel (5),
        mGroundFogVerticalDecay (0.2),
        mGroundFogColour (ColourValue::Black)
    {
	}

	DepthComposer::~DepthComposer()
    {
        destroyAllViewportInstances();
	}

    void DepthComposer::setDebugDepthRender (bool value)
    {
        if (mDebugDepthRender == value) {
            return;
        }
        mDebugDepthRender = value;
        onCompositorMaterialChanged ();
    }

    void DepthComposer::setSkyDomeHazeEnabled (bool value)
    {
        if (mSkyDomeHazeEnabled == value) {
            return;
        }
        mSkyDomeHazeEnabled = value;
        onCompositorMaterialChanged ();
    }

    void DepthComposer::setGroundFogEnabled (bool value)
    {
        if (mGroundFogEnabled == value) {
            return;
        }
        mGroundFogEnabled = value;
        onCompositorMaterialChanged ();
    }

    const String& DepthComposer::getCompositorName ()
    {
        // Constant Ogre::Strings for names.
        static const Ogre::String CompositorName_DebugDepthRender =
                          "Caelum/DepthComposer_DebugDepthRender";
        static const Ogre::String CompositorName_Dummy =
                          "Caelum/DepthComposer_Dummy";
        static const Ogre::String CompositorName_ExpGroundFog =
                          "Caelum/DepthComposer_ExpGroundFog";
        static const Ogre::String CompositorName_SkyDomeHaze =
                          "Caelum/DepthComposer_SkyDomeHaze";
        static const Ogre::String CompositorName_SkyDomeHaze_ExpGroundFog =
                          "Caelum/DepthComposer_SkyDomeHaze_ExpGroundFog";

        // Should probably build materials and compositors by hand.
        if (mDebugDepthRender) {
            return CompositorName_DebugDepthRender;
        } else if (mSkyDomeHazeEnabled == false && mGroundFogEnabled == false) {
            return CompositorName_Dummy;
        } else if (mSkyDomeHazeEnabled == false && mGroundFogEnabled == true) {
            return CompositorName_ExpGroundFog;
        } else if (mSkyDomeHazeEnabled == true && mGroundFogEnabled == false) {
            return CompositorName_SkyDomeHaze;
        } else if (mSkyDomeHazeEnabled == true && mGroundFogEnabled == true) {
            return CompositorName_SkyDomeHaze_ExpGroundFog;
        } else {
            assert (0);
            return CompositorName_Dummy;
        }
    }

    void DepthComposer::onCompositorMaterialChanged ()
    {
        ViewportInstanceMap::const_iterator it;
        ViewportInstanceMap::const_iterator begin = mViewportInstanceMap.begin();
        ViewportInstanceMap::const_iterator end = mViewportInstanceMap.end();
        for (it = begin; it != end; ++it) {
            it->second->removeCompositor ();
            it->second->addCompositor ();
        }
    }

	void DepthComposer::update ()
    {
        ViewportInstanceMap::const_iterator it;
        ViewportInstanceMap::const_iterator begin = mViewportInstanceMap.begin();
        ViewportInstanceMap::const_iterator end = mViewportInstanceMap.end();
        for (it = begin; it != end; ++it) {
            assert(it->first == it->second->getViewport());
            it->second->_update ();
        }
	}

    DepthComposerInstance::DepthComposerInstance
    (
        DepthComposer* parent,
        Ogre::Viewport* viewport
    ):
        mParent(parent),
        mViewport(viewport),
        mCompInst(0)
    {
        LogManager::getSingleton().logMessage (
                "Caelum::DepthComposer: Attaching screen-space fog instance"
                " to viewport \'" + StringConverter::toString ((long)getViewport ()) + "\'"
                " of render target \'" + getViewport()->getTarget ()->getName () + "\'");

        addCompositor ();
        mDepthRenderer.reset (new DepthRenderer (getViewport ()));
    }

    DepthComposerInstance::~DepthComposerInstance()
    {
        removeCompositor ();
        mDepthRenderer.reset ();

        LogManager::getSingleton().logMessage (
                "Caelum::DepthComposer: Detached screen-space fog instance"
                " from viewport \'" + StringConverter::toString ((long)getViewport ()) + "\'"
                " of render target \'" + getViewport()->getTarget ()->getName () + "\'");
    }

    void DepthComposerInstance::addCompositor ()
    {
        CompositorManager* compMgr = CompositorManager::getSingletonPtr();

        const String& compositorName = getParent ()->getCompositorName ();
        mCompInst = compMgr->addCompositor(mViewport, compositorName);
        if (!mCompInst) {
            CAELUM_THROW_UNSUPPORTED_EXCEPTION (
                    "Can't add \'" + compositorName + "\' compositor.",
                    "DepthComposer");
        }
        assert(mCompInst);
		mCompInst->setEnabled (true);
		mCompInst->addListener (this);
    }

    void DepthComposerInstance::removeCompositor ()
    {
        CompositorManager* compMgr = CompositorManager::getSingletonPtr();
        compMgr->removeCompositor (mViewport, mCompInst->getCompositor ()->getName ());
        mCompInst = 0;
    }

	void DepthComposerInstance::notifyMaterialSetup(uint pass_id, Ogre::MaterialPtr &mat)
	{
        //LogManager::getSingleton ().logMessage (
        //            "Caelum::DepthComposer: Material setup");

        Pass* pass = mat->getBestTechnique ()->getPass (0);

        TextureUnitState *depthTus = pass->getTextureUnitState(1);
        if (depthTus->getTextureName () != mDepthRenderer->getDepthRenderTexture ()->getName()) {
            depthTus->setTextureName (mDepthRenderer->getDepthRenderTexture ()->getName ());
            LogManager::getSingleton ().logMessage (
                        "Caelum::DepthComposer: Assigned depth texture in compositor material");
        }

        mParams.setup(pass->getFragmentProgramParameters ());
	}

    void DepthComposerInstance::Params::setup(Ogre::GpuProgramParametersSharedPtr fpParams)
    {
        this->fpParams = fpParams;
        invViewProjMatrix.bind(fpParams, "invViewProjMatrix");
        worldCameraPos.bind(fpParams, "worldCameraPos");
        groundFogDensity.bind(fpParams, "groundFogDensity");
        groundFogVerticalDecay.bind(fpParams, "groundFogVerticalDecay");
        groundFogBaseLevel.bind(fpParams, "groundFogBaseLevel");
        groundFogColour.bind(fpParams, "groundFogColour");
        sunDirection.bind(fpParams, "sunDirection");
        hazeColour.bind(fpParams, "hazeColour");
    }

	void DepthComposerInstance::notifyMaterialRender(uint pass_id, Ogre::MaterialPtr &mat)
	{
        Camera* camera = getViewport ()->getCamera ();

        assert(mParams.fpParams == mat->getBestTechnique ()->getPass (0)->getFragmentProgramParameters ());

        // Auto param in a compositor does not use the external camera.
        // This means that sending matrices as auto_param will not work as expected.
        // Do it manually instead.
        Matrix4 projMatrix = camera->getProjectionMatrixWithRSDepth();
        Matrix4 viewMatrix = camera->getViewMatrix();

        mParams.invViewProjMatrix.set(mParams.fpParams, (projMatrix * viewMatrix).inverse());

        mParams.worldCameraPos.set(mParams.fpParams, camera->getDerivedPosition ());

        mParams.groundFogDensity.set(mParams.fpParams, getParent ()->getGroundFogDensity ());
        mParams.groundFogVerticalDecay.set(mParams.fpParams, getParent ()->getGroundFogVerticalDecay ());
        mParams.groundFogBaseLevel.set(mParams.fpParams, getParent ()->getGroundFogBaseLevel ());
        mParams.groundFogColour.set(mParams.fpParams, getParent ()->getGroundFogColour ());

        mParams.sunDirection.set(mParams.fpParams, getParent ()->getSunDirection ());
        mParams.hazeColour.set(mParams.fpParams, getParent ()->getHazeColour ());
	}

	void DepthComposerInstance::_update ()
    {
        mDepthRenderer->update ();
    }

    DepthComposerInstance* DepthComposer::createViewportInstance(Ogre::Viewport* vp)
    {
        ViewportInstanceMap::const_iterator it = mViewportInstanceMap.find(vp);
        if (it == mViewportInstanceMap.end()) {
            std::auto_ptr<DepthComposerInstance> inst(new DepthComposerInstance(this, vp));
            mViewportInstanceMap.insert(std::make_pair(vp, inst.get()));
            // hold instance until successfully added to map.
            return inst.release();
        } else {
            return it->second;
        }
    }
    
    DepthComposerInstance* DepthComposer::getViewportInstance(Ogre::Viewport* vp) {
        ViewportInstanceMap::iterator it = mViewportInstanceMap.find(vp);
        if (it != mViewportInstanceMap.end()) {
            return it->second;
        } else {
            return 0;
        }
    }

    void DepthComposer::destroyViewportInstance(Viewport* vp)
    {
        ViewportInstanceMap::iterator it = mViewportInstanceMap.find(vp);
        if (it != mViewportInstanceMap.end()) {
            DepthComposerInstance* inst = it->second;
            delete inst;
            mViewportInstanceMap.erase(it);
        }
    }

    void DepthComposer::destroyAllViewportInstances() {
        ViewportInstanceMap::const_iterator it;
        ViewportInstanceMap::const_iterator begin = mViewportInstanceMap.begin();
        ViewportInstanceMap::const_iterator end = mViewportInstanceMap.end();
        for (it = begin; it != end; ++it) {
            assert(it->first == it->second->getViewport());
            delete it->second;
        }
        mViewportInstanceMap.clear();
    }

	const String DepthRenderer::DEFAULT_CUSTOM_DEPTH_SCHEME_NAME = "CaelumDepth";

    DepthRenderer::DepthRenderer
    (
        Viewport* masterViewport
    ):
        mMasterViewport (masterViewport),
        mDepthRenderViewport (0),
        mDepthRenderingNow (false),
        mViewportVisibilityMask (~0),
        mUseCustomDepthScheme (true),
        mCustomDepthSchemeName (DEFAULT_CUSTOM_DEPTH_SCHEME_NAME)
    {
        disableRenderGroupRangeFilter ();

		Ogre::String uniqueId = Ogre::StringConverter::toString ((size_t)this);

        // Not cloned!
        mDepthRenderMaterial = MaterialManager::getSingleton ().getByName ("Caelum/DepthRender");
        mDepthRenderMaterial->load();
        if (!mDepthRenderMaterial->getBestTechnique ()) {
            CAELUM_THROW_UNSUPPORTED_EXCEPTION (
                    "Can't load depth render material: " +
                    mDepthRenderMaterial->getUnsupportedTechniquesExplanation(),
                    "DepthComposer");
        }

        TextureManager* texMgr = TextureManager::getSingletonPtr();

        int width = getMasterViewport ()->getActualWidth ();
        int height = getMasterViewport ()->getActualHeight ();
        LogManager::getSingleton ().logMessage (
                    "Caelum::DepthRenderer: Creating depth render texture size " +
                    StringConverter::toString (width) + 
                    "x" + 
                    StringConverter::toString (height));

        PixelFormat desiredFormat = PF_FLOAT32_R;
        PixelFormat requestFormat = desiredFormat;
        if (texMgr->isFormatSupported (TEX_TYPE_2D, desiredFormat, TU_RENDERTARGET)) {
            LogManager::getSingleton ().logMessage (
                    "Caelum::DepthRenderer: RenderSystem has native support for " +
                    PixelUtil::getFormatName (desiredFormat));
        } else if (texMgr->isEquivalentFormatSupported (TEX_TYPE_2D, desiredFormat, TU_RENDERTARGET)) {
            PixelFormat equivFormat = texMgr->getNativeFormat (TEX_TYPE_2D, desiredFormat, TU_RENDERTARGET);
            LogManager::getSingleton ().logMessage (
                    "Caelum::DepthRenderer: RenderSystem supports " +
                    PixelUtil::getFormatName (equivFormat) +
                    " instead of " +
                    PixelUtil::getFormatName (desiredFormat));
            requestFormat = equivFormat;
        } else {
            CAELUM_THROW_UNSUPPORTED_EXCEPTION (
                    PixelUtil::getFormatName(desiredFormat) + " or equivalent not supported",
                    "DepthRenderer");
        }

        if (texMgr->isHardwareFilteringSupported (TEX_TYPE_2D, requestFormat, TU_RENDERTARGET)) {
            LogManager::getSingleton ().logMessage (
                    "Caelum::DepthRenderer: RenderSystem supports hardware filtering for " +
                    PixelUtil::getFormatName (requestFormat));
        } else {
            LogManager::getSingleton ().logMessage (
                    "Caelum::DepthRenderer: RenderSystem does not support hardware filtering for " +
                    PixelUtil::getFormatName (requestFormat));
        }

        // Create depth texture.
        // This depends on the size of the viewport.
        mDepthRenderTexture = texMgr->createManual(
                "Caelum/DepthComposer/" + uniqueId + "/DepthTexture",
                Caelum::RESOURCE_GROUP_NAME,
                TEX_TYPE_2D,
                width, height, 1,
                0,
                requestFormat,
                TU_RENDERTARGET,
                0);

        assert(getDepthRenderTarget());

        // Should be the same format
        LogManager::getSingleton().logMessage (
                "Caelum::DepthRenderer: Created depth render texture"
                " actual format " + PixelUtil::getFormatName (getDepthRenderTexture()->getFormat ()) +
                " desired format " + PixelUtil::getFormatName (getDepthRenderTexture()->getDesiredFormat ()));

        // We do our updates by hand.
        getDepthRenderTarget()->setAutoUpdated (false);

        // Viewport for the depth rtt. Don't set camera here; it can mess Camera::getViewport();
        mDepthRenderViewport = getDepthRenderTarget()->addViewport(0);
        getDepthRenderViewport ()->setShadowsEnabled (false);
        getDepthRenderViewport ()->setOverlaysEnabled (false);
        getDepthRenderViewport ()->setClearEveryFrame (true);

        // Depth buffer values range from 0 to 1 in both OpenGL and Directx; unless depth ranges are used.
        // Clear to the maximum value.
        getDepthRenderViewport ()->setBackgroundColour (Ogre::ColourValue (1, 1, 1, 1));
    }

    DepthRenderer::~DepthRenderer()
    {
        TextureManager* texMgr = TextureManager::getSingletonPtr();

        // Destroy render texture.
        if (!mDepthRenderTexture.isNull ()) {
            texMgr->remove (mDepthRenderTexture->getHandle ());
            mDepthRenderTexture.setNull ();
        }
    }

    void DepthRenderer::update ()
    {
        Camera* camera = getMasterViewport ()->getCamera ();
        Viewport* oldCameraViewport = camera->getViewport ();
        SceneManager *sceneManager = camera->getSceneManager ();

        assert (oldCameraViewport == getMasterViewport ());
        assert (getDepthRenderViewport ()->getActualWidth () == getMasterViewport()->getActualWidth ());
        assert (getDepthRenderViewport ()->getActualHeight () == getMasterViewport()->getActualHeight ());

        getDepthRenderViewport ()->setVisibilityMask (mViewportVisibilityMask);
        getDepthRenderViewport ()->setCamera (camera);
        if (this->getUseCustomDepthScheme ()) {
            getDepthRenderViewport ()->setMaterialScheme (this->getCustomDepthSchemeName ());
        }

        // Restore old listener after we're done.
        // Hopefully this will not break horribly.
        RenderQueue::RenderableListener* oldListener = sceneManager->getRenderQueue ()->getRenderableListener();
        if (oldListener) {
            //LogManager::getSingleton ().logMessage (
            //        "Caelum: Found another render queue listener. This could be bad.");
        }
        sceneManager->getRenderQueue ()->setRenderableListener (this);

        mDepthRenderingNow = true;
        //LogManager::getSingleton ().logMessage ("Caelum: Begin depth rendering");
        getDepthRenderTarget ()->update ();
        //LogManager::getSingleton ().logMessage ("Caelum: End depth rendering");
        mDepthRenderingNow = false;

        sceneManager->getRenderQueue ()->setRenderableListener (oldListener);
        oldListener = 0;

        // Restore the camera's viewport. Ogre compositors do the same thing.
        camera->_notifyViewport (oldCameraViewport);
    }

#if OGRE_VERSION < 0x00010600
    bool DepthRenderer::renderableQueued(
                Ogre::Renderable* rend,
                Ogre::uint8 groupId,
                Ogre::ushort priority, 
                Ogre::Technique** ppTech)
#else
    bool DepthRenderer::renderableQueued(
                Ogre::Renderable* rend,
                Ogre::uint8 groupId,
                Ogre::ushort priority, 
                Ogre::Technique** ppTech,
                Ogre::RenderQueue* pQueue)
#endif // OGRE_VERSION
    {
        assert (mDepthRenderingNow);

        /*
        LogManager::getSingleton ().logMessage (
                "Caelum: Renderable queued"
                " group " + StringConverter::toString (groupId) + 
                " priority " + StringConverter::toString (priority));
        */
        if (groupId < mMinRenderGroupId || groupId > mMaxRenderGroupId) {
            return false;
        }

        if (this->getUseCustomDepthScheme () && (*ppTech)->getSchemeName () == this->getCustomDepthSchemeName ()) {
            /*
            LogManager::getSingleton().getDefaultLog()->logMessage (
                    "Custom scheme with tech " + (*ppTech)->getName () + 
                    " passCount " + StringConverter::toString ((*ppTech)->getNumPasses ()) +
                    " vp " + (*ppTech)->getPass (0)->getVertexProgramName () + 
                    " fp " + (*ppTech)->getPass (0)->getFragmentProgramName ());
             */
            return true;
        }

        // Get depth material
        Material* depthMaterial = getDepthRenderMaterial ();
        Technique* tech = depthMaterial->getBestTechnique ();

        // Replace ALL techniques.
        *ppTech = tech;
        return true;
    }

    void DepthRenderer::setRenderGroupRangeFilter (int minGroup, int maxGroup)
    {
        mMinRenderGroupId = minGroup;
        mMaxRenderGroupId = maxGroup;
    }

    void DepthRenderer::disableRenderGroupRangeFilter()
    {
        setRenderGroupRangeFilter(Ogre::RENDER_QUEUE_BACKGROUND, Ogre::RENDER_QUEUE_MAX);
    }
}
