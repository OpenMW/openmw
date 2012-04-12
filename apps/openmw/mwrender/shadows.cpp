#include "shadows.hpp"

#include <components/settings/settings.hpp>
#include <openengine/ogre/renderer.hpp>

#include <OgreSceneManager.h>
#include <OgreColourValue.h>
#include <OgreShadowCameraSetupLiSPSM.h>
#include <OgreShadowCameraSetupPSSM.h>
#include <OgreHardwarePixelBuffer.h>

#include <OgreOverlayContainer.h>
#include <OgreOverlayManager.h>

#include "renderconst.hpp"

using namespace Ogre;
using namespace MWRender;

Shadows::Shadows(OEngine::Render::OgreRenderer* rend) :
    mShadowFar(1000), mFadeStart(0.9)
{
    mRendering = rend;
    mSceneMgr = mRendering->getScene();
    recreate();
}

void Shadows::recreate()
{
    bool enabled = Settings::Manager::getBool("enabled", "Shadows");

    // Split shadow maps are currently disabled because the terrain cannot cope with them
    // (Too many texture units) Solution would be a multi-pass terrain material
    bool split = Settings::Manager::getBool("split", "Shadows");
    //const bool split = false;

    if (!enabled)
    {
        mSceneMgr->setShadowTechnique(SHADOWTYPE_NONE);
        return;
    }

    int texsize = Settings::Manager::getInt("texture size", "Shadows");
    mSceneMgr->setShadowTextureSize(texsize);

    mSceneMgr->setShadowTechnique(SHADOWTYPE_TEXTURE_MODULATIVE_INTEGRATED);

    // no point light shadows, i'm afraid. might revisit this with Deferred Shading
    mSceneMgr->setShadowTextureCountPerLightType(Light::LT_POINT, 0);

    mSceneMgr->setShadowTextureCountPerLightType(Light::LT_DIRECTIONAL, split ? 3 : 1);
    mSceneMgr->setShadowTextureCount(split ? 3 : 1);

    mSceneMgr->setShadowTextureSelfShadow(true);
    mSceneMgr->setShadowCasterRenderBackFaces(true);
    mSceneMgr->setShadowTextureCasterMaterial("depth_shadow_caster");
    mSceneMgr->setShadowTexturePixelFormat(PF_FLOAT32_R);
    mSceneMgr->setShadowDirLightTextureOffset(0.9);
    mSceneMgr->setShadowDirectionalLightExtrusionDistance(1000000);
    mSceneMgr->setShowDebugShadows(true);

    mShadowFar = split ? Settings::Manager::getInt("split shadow distance", "Shadows") : Settings::Manager::getInt("shadow distance", "Shadows");
    mSceneMgr->setShadowFarDistance(mShadowFar);

    mFadeStart = Settings::Manager::getFloat("fade start", "Shadows");

    ShadowCameraSetupPtr shadowCameraSetup;
    if (split)
    {
        mPSSMSetup = new PSSMShadowCameraSetup();
        mPSSMSetup->setSplitPadding(5);
        mPSSMSetup->calculateSplitPoints(3, mRendering->getCamera()->getNearClipDistance(), mShadowFar);

        const Real adjustFactors[3] = {64, 64, 64};
        for (int i=0; i < 3; ++i)
        {
            mPSSMSetup->setOptimalAdjustFactor(i, adjustFactors[i]);
            /*if (i==0)
                mSceneMgr->setShadowTextureConfig(i, texsize, texsize, Ogre::PF_FLOAT32_R);
            else if (i ==1)
                mSceneMgr->setShadowTextureConfig(i, texsize/2, texsize/2, Ogre::PF_FLOAT32_R);
            else if (i ==2)
                mSceneMgr->setShadowTextureConfig(i, texsize/4, texsize/4, Ogre::PF_FLOAT32_R);*/
        }

        shadowCameraSetup = ShadowCameraSetupPtr(mPSSMSetup);
    }
    else
    {
        LiSPSMShadowCameraSetup* lispsmSetup = new LiSPSMShadowCameraSetup();
        lispsmSetup->setOptimalAdjustFactor(2);
        //lispsmSetup->setCameraLightDirectionThreshold(Degree(0));
        //lispsmSetup->setUseAggressiveFocusRegion(false);
        shadowCameraSetup = ShadowCameraSetupPtr(lispsmSetup);
    }
    mSceneMgr->setShadowCameraSetup(shadowCameraSetup);

    // Set visibility mask for the shadow render textures
    int visibilityMask = RV_Actors * Settings::Manager::getBool("actor shadows", "Shadows")
                            + (RV_Statics + RV_StaticsSmall) * Settings::Manager::getBool("statics shadows", "Shadows")
                            + RV_Misc * Settings::Manager::getBool("misc shadows", "Shadows");

    for (int i = 0; i < (split ? 3 : 1); ++i)
    {
        TexturePtr shadowTexture = mSceneMgr->getShadowTexture(i);
        Viewport* vp = shadowTexture->getBuffer()->getRenderTarget()->getViewport(0);
        vp->setVisibilityMask(visibilityMask);
    }

    // --------------------------------------------------------------------------------------------------------------------
    // --------------------------- Debug overlays to display the content of shadow maps -----------------------------------
    // --------------------------------------------------------------------------------------------------------------------
    /*
	OverlayManager& mgr = OverlayManager::getSingleton();
	Overlay* overlay;
	
	// destroy if already exists
	if (overlay = mgr.getByName("DebugOverlay"))
		mgr.destroy(overlay);
		
	overlay = mgr.create("DebugOverlay");
	for (size_t i = 0; i < (split ? 3 : 1); ++i) {
		TexturePtr tex = mRendering->getScene()->getShadowTexture(i);
		
		// Set up a debug panel to display the shadow
		
		if (MaterialManager::getSingleton().resourceExists("Ogre/DebugTexture" + StringConverter::toString(i)))
			MaterialManager::getSingleton().remove("Ogre/DebugTexture" + StringConverter::toString(i));
		MaterialPtr debugMat = MaterialManager::getSingleton().create(
			"Ogre/DebugTexture" + StringConverter::toString(i), 
			ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
			
		debugMat->getTechnique(0)->getPass(0)->setLightingEnabled(false);
		TextureUnitState *t = debugMat->getTechnique(0)->getPass(0)->createTextureUnitState(tex->getName());
		t->setTextureAddressingMode(TextureUnitState::TAM_CLAMP);

		OverlayContainer* debugPanel;
		
		// destroy container if exists
		try
		{
			if (debugPanel = 
				static_cast<OverlayContainer*>(
					mgr.getOverlayElement("Ogre/DebugTexPanel" + StringConverter::toString(i)
				)))
				mgr.destroyOverlayElement(debugPanel);
		}
		catch (Ogre::Exception&) {}
		
		debugPanel = (OverlayContainer*)
			(OverlayManager::getSingleton().createOverlayElement("Panel", "Ogre/DebugTexPanel" + StringConverter::toString(i)));
		debugPanel->_setPosition(0.8, i*0.25);
		debugPanel->_setDimensions(0.2, 0.24);
		debugPanel->setMaterialName(debugMat->getName());
		debugPanel->show();
		overlay->add2D(debugPanel);
		overlay->show();
	}
    */
}

PSSMShadowCameraSetup* Shadows::getPSSMSetup()
{
    return mPSSMSetup;
}

float Shadows::getShadowFar() const
{
    return mShadowFar;
}

float Shadows::getFadeStart() const
{
    return mFadeStart;
}
