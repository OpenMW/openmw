#include "fader.hpp"

#include <OgreMaterial.h>
#include <OgreTechnique.h>
#include <OgreMaterialManager.h>
#include <OgreResourceGroupManager.h>
#include <OgreRectangle2D.h>
#include <OgreSceneManager.h>
#include <OgreSceneNode.h>


using namespace Ogre;
using namespace OEngine::Render;

Fader::Fader(Ogre::SceneManager* sceneMgr)
    : mSceneMgr(sceneMgr)
    , mMode(FadingMode_In)
    , mRemainingTime(0.f)
    , mTargetTime(0.f)
    , mTargetAlpha(0.f)
    , mCurrentAlpha(0.f)
    , mStartAlpha(0.f)
    , mFactor(1.f)
{
    // Create the fading material
    MaterialPtr material = MaterialManager::getSingleton().create("FadeInOutMaterial", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );
    Pass* pass = material->getTechnique(0)->getPass(0);
    pass->setSceneBlending(SBT_TRANSPARENT_ALPHA);
    pass->setDepthWriteEnabled (false);
    mFadeTextureUnit = pass->createTextureUnitState("black.png");
    mFadeTextureUnit->setColourOperationEx(LBX_SOURCE1, LBS_MANUAL, LBS_CURRENT, ColourValue(0.f, 0.f, 0.f)); // always black colour    

    mRectangle = new Ogre::Rectangle2D(true);
    mRectangle->setCorners(-1.0, 1.0, 1.0, -1.0);
    mRectangle->setMaterial("FadeInOutMaterial");
    mRectangle->setRenderQueueGroup(Ogre::RENDER_QUEUE_OVERLAY-1);
    // Use infinite AAB to always stay visible
    Ogre::AxisAlignedBox aabInf;
    aabInf.setInfinite();
    mRectangle->setBoundingBox(aabInf);
    // Attach background to the scene
    Ogre::SceneNode* node = mSceneMgr->getRootSceneNode()->createChildSceneNode();
    node->attachObject(mRectangle);
    mRectangle->setVisible(false);
    mRectangle->setVisibilityFlags (2048);
}

Fader::~Fader()
{
    delete mRectangle;
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
                
        mRemainingTime -= dt;
    }
    
    if (1.f-((1.f-mCurrentAlpha) * mFactor) == 0.f)
        mRectangle->setVisible(false);
    else
        applyAlpha();
}

void Fader::applyAlpha()
{
    mRectangle->setVisible(true);
    mFadeTextureUnit->setAlphaOperation(LBX_SOURCE1, LBS_MANUAL, LBS_CURRENT, 1.f-((1.f-mCurrentAlpha) * mFactor));
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
