#include "fader.hpp"

#include <OgreOverlayManager.h>
#include <OgreOverlayContainer.h>
#include <OgreOverlay.h>
#include <OgreMaterial.h>
#include <OgreTechnique.h>
#include <OgreMaterialManager.h>
#include <OgreResourceGroupManager.h>

#define FADE_OVERLAY_NAME       "FadeInOutOverlay"
#define FADE_OVERLAY_PANEL_NAME "FadeInOutOverlayPanel"
#define FADE_MATERIAL_NAME      "FadeInOutMaterial"

using namespace Ogre;
using namespace OEngine::Render;

Fader::Fader() : 
    mMode(FadingMode_In),
    mRemainingTime(0.f),
    mTargetTime(0.f),
    mTargetAlpha(0.f),
    mCurrentAlpha(0.f),
    mStartAlpha(0.f)
{

    // Create the fading material
    MaterialPtr material = MaterialManager::getSingleton().create( FADE_MATERIAL_NAME, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );
    Pass* pass = material->getTechnique(0)->getPass(0);
    pass->setSceneBlending(SBT_TRANSPARENT_ALPHA);
    mFadeTextureUnit = pass->createTextureUnitState();
    mFadeTextureUnit->setColourOperationEx(LBX_SOURCE1, LBS_MANUAL, LBS_CURRENT, ColourValue(0.f, 0.f, 0.f)); // always black colour    

    // Create the overlay
    OverlayManager& ovm = OverlayManager::getSingleton();

    mOverlay = ovm.create( FADE_OVERLAY_NAME );
    
    OverlayContainer* overlay_panel;
    overlay_panel = (OverlayContainer*)ovm.createOverlayElement("Panel", FADE_OVERLAY_PANEL_NAME);
    
    // position it over the whole screen
    overlay_panel->_setPosition(0, 0);
    overlay_panel->_setDimensions(1, 1);
    
    overlay_panel->setMaterialName( FADE_MATERIAL_NAME );
    overlay_panel->show();
    mOverlay->add2D(overlay_panel);
    mOverlay->hide();
}

void Fader::update(float dt)
{    
    if (mRemainingTime > 0)
    {
        if (mMode == FadingMode_In)
        {
            mCurrentAlpha -= dt/mTargetTime * (mStartAlpha-mTargetAlpha);
            if (mCurrentAlpha < mTargetAlpha) mCurrentAlpha = mTargetAlpha;
        }
        else if (mMode == FadingMode_Out)
        {
            mCurrentAlpha += dt/mTargetTime * (mTargetAlpha-mStartAlpha);
            if (mCurrentAlpha > mTargetAlpha) mCurrentAlpha = mTargetAlpha;
        }
        
        applyAlpha();
        
        mRemainingTime -= dt;
    }
    
   if (mCurrentAlpha == 0.f) mOverlay->hide();
}

void Fader::applyAlpha()
{
    mOverlay->show();
    mFadeTextureUnit->setAlphaOperation(LBX_SOURCE1, LBS_MANUAL, LBS_CURRENT, mCurrentAlpha);
}

void Fader::fadeIn(float time)
{
    if (time<0.f) return;
    if (time==0.f)
    {
        mCurrentAlpha = 0.f;
        applyAlpha();
        return;
    }
        
    mStartAlpha = mCurrentAlpha;
    mTargetAlpha = 0.f;
    mMode = FadingMode_In;
    mTargetTime = time;
    mRemainingTime = time;
}

void Fader::fadeOut(const float time)
{
    if (time<0.f) return;
    if (time==0.f)
    {
        mCurrentAlpha = 1.f;
        applyAlpha();
        return;
    }
        
    mStartAlpha = mCurrentAlpha;
    mTargetAlpha = 1.f;
    mMode = FadingMode_Out;
    mTargetTime = time;
    mRemainingTime = time;
}

void Fader::fadeTo(const int percent, const float time)
{
    if (time<0.f) return;
    if (time==0.f)
    {
        mCurrentAlpha = percent/100.f;
        applyAlpha();
        return;
    }
    
    mStartAlpha = mCurrentAlpha;
    mTargetAlpha = percent/100.f;
    
    if (mTargetAlpha == mStartAlpha) return;
    else if (mTargetAlpha > mStartAlpha) mMode = FadingMode_Out;
    else mMode = FadingMode_In;
    
    mTargetTime = time;
    mRemainingTime = time;
}
