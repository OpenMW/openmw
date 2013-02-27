#include "ripplesimulation.hpp"

#include <OgreTextureManager.h>
#include <OgreStringConverter.h>
#include <OgreHardwarePixelBuffer.h>
#include <OgreRoot.h>

#include <extern/shiny/Main/Factory.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/player.hpp"

namespace MWRender
{


RippleSimulation::RippleSimulation(Ogre::SceneManager* mainSceneManager)
    : mMainSceneMgr(mainSceneManager),
      mTime(0),
      mCurrentFrameOffset(0,0),
      mPreviousFrameOffset(0,0),
      mRippleCenter(0,0),
      mTextureSize(512),
      mRippleAreaLength(1000),
      mImpulseSize(20),
      mTexelOffset(0,0),
      mFirstUpdate(true)
{
    Ogre::AxisAlignedBox aabInf;
    aabInf.setInfinite();

    mSceneMgr = Ogre::Root::getSingleton().createSceneManager(Ogre::ST_GENERIC);

    mCamera = mSceneMgr->createCamera("RippleCamera");

    mRectangle = new Ogre::Rectangle2D(true);
    mRectangle->setBoundingBox(aabInf);
    mRectangle->setCorners(-1.0, 1.0, 1.0, -1.0, false);
    Ogre::SceneNode* node = mSceneMgr->getRootSceneNode()->createChildSceneNode();
    node->attachObject(mRectangle);

    mImpulse = new Ogre::Rectangle2D(true);
    mImpulse->setCorners(-0.1, 0.1, 0.1, -0.1, false);
    mImpulse->setBoundingBox(aabInf);
    mImpulse->setMaterial("AddImpulse");
    Ogre::SceneNode* impulseNode = mSceneMgr->getRootSceneNode()->createChildSceneNode();
    impulseNode->attachObject(mImpulse);

    //float w=0.05;
    for (int i=0; i<4; ++i)
    {
        Ogre::TexturePtr texture;
        if (i != 3)
            texture = Ogre::TextureManager::getSingleton().createManual("RippleHeight" + Ogre::StringConverter::toString(i),
                Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, Ogre::TEX_TYPE_2D, mTextureSize, mTextureSize, 1, 0, Ogre::PF_R8G8B8, Ogre::TU_RENDERTARGET);
        else
            texture = Ogre::TextureManager::getSingleton().createManual("RippleNormal",
                Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, Ogre::TEX_TYPE_2D, mTextureSize, mTextureSize, 1, 0, Ogre::PF_R8G8B8, Ogre::TU_RENDERTARGET);


        Ogre::RenderTexture* rt = texture->getBuffer()->getRenderTarget();
        rt->removeAllViewports();
        rt->addViewport(mCamera);
        rt->setAutoUpdated(false);
        rt->getViewport(0)->setClearEveryFrame(false);

        // debug overlay
        /*
        Ogre::Rectangle2D* debugOverlay = new Ogre::Rectangle2D(true);
        debugOverlay->setCorners(w*2-1, 0.9, (w+0.18)*2-1, 0.4, false);
        w += 0.2;
        debugOverlay->setBoundingBox(aabInf);
        Ogre::SceneNode* debugNode = mMainSceneMgr->getRootSceneNode()->createChildSceneNode();
        debugNode->attachObject(debugOverlay);

        Ogre::MaterialPtr debugMaterial = Ogre::MaterialManager::getSingleton().create("RippleDebug" + Ogre::StringConverter::toString(i),
            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

        if (i != 3)
            debugMaterial->getTechnique(0)->getPass(0)->createTextureUnitState("RippleHeight" + Ogre::StringConverter::toString(i));
        else
            debugMaterial->getTechnique(0)->getPass(0)->createTextureUnitState("RippleNormal");
        debugMaterial->getTechnique(0)->getPass(0)->setLightingEnabled(false);

        debugOverlay->setMaterial("RippleDebug" + Ogre::StringConverter::toString(i));
        */

        mRenderTargets[i] = rt;
        mTextures[i] = texture;
    }

    sh::Factory::getInstance().setSharedParameter("rippleTextureSize", sh::makeProperty<sh::Vector4>(
        new sh::Vector4(1.0/512, 1.0/512, 512, 512)));
    sh::Factory::getInstance().setSharedParameter("rippleCenter", sh::makeProperty<sh::Vector3>(
         new sh::Vector3(0, 0, 0)));
    sh::Factory::getInstance().setSharedParameter("rippleAreaLength", sh::makeProperty<sh::FloatValue>(
         new sh::FloatValue(mRippleAreaLength)));

}

RippleSimulation::~RippleSimulation()
{
    delete mRectangle;

    Ogre::Root::getSingleton().destroySceneManager(mSceneMgr);
}

void RippleSimulation::update(float dt, Ogre::Vector2 position)
{
    // try to keep 20 fps
    mTime += dt;

    while (mTime >= 1/20.0 || mFirstUpdate)
    {
        mPreviousFrameOffset = mCurrentFrameOffset;

        mCurrentFrameOffset = position - mRippleCenter;
        // add texel offsets from previous frame.
        mCurrentFrameOffset += mTexelOffset;

        mTexelOffset = Ogre::Vector2(std::fmod(mCurrentFrameOffset.x, 1.0f/mTextureSize),
                                          std::fmod(mCurrentFrameOffset.y, 1.0f/mTextureSize));

        // now subtract new offset in order to snap to texels
        mCurrentFrameOffset -= mTexelOffset;

        // texture coordinate space
        mCurrentFrameOffset /= mRippleAreaLength;

        mRippleCenter = position;

        addImpulses();
        waterSimulation();
        heightMapToNormalMap();

        swapHeightMaps();
        if (!mFirstUpdate)
            mTime -= 1/20.0;
        else
            mFirstUpdate = false;
    }

    sh::Factory::getInstance().setSharedParameter("rippleCenter", sh::makeProperty<sh::Vector3>(
         new sh::Vector3(mRippleCenter.x + mTexelOffset.x, mRippleCenter.y + mTexelOffset.y, 0)));
}

void RippleSimulation::addImpulses()
{
    mRectangle->setVisible(false);
    mImpulse->setVisible(true);

    /// \todo it should be more efficient to render all emitters at once
    for (std::vector<Emitter>::iterator it=mEmitters.begin(); it !=mEmitters.end(); ++it)
    {
        if (it->mPtr == MWBase::Environment::get().getWorld ()->getPlayer ().getPlayer ())
        {
            // fetch a new ptr (to handle cell change etc)
            // for non-player actors this is done in updateObjectCell
            it->mPtr = MWBase::Environment::get().getWorld ()->getPlayer ().getPlayer ();
        }
        float* _currentPos = it->mPtr.getRefData().getPosition().pos;
        Ogre::Vector3 currentPos (_currentPos[0], _currentPos[1], _currentPos[2]);

        if ( (currentPos - it->mLastEmitPosition).length() > 2
            && MWBase::Environment::get().getWorld ()->isUnderwater (it->mPtr.getCell(), currentPos))
        {
            it->mLastEmitPosition = currentPos;

            Ogre::Vector2 pos (currentPos.x, currentPos.y);
            pos -= mRippleCenter;
            pos /= mRippleAreaLength;
            float size = mImpulseSize / mRippleAreaLength;
            mImpulse->setCorners(pos.x-size, pos.y+size, pos.x+size, pos.y-size, false);

            // don't render if we are offscreen
            if (pos.x - size >= 1.0 || pos.y+size <= -1.0 || pos.x+size <= -1.0 || pos.y-size >= 1.0)
                continue;
            mRenderTargets[1]->update();
        }
    }

    mImpulse->setVisible(false);
    mRectangle->setVisible(true);
}

void RippleSimulation::waterSimulation()
{
    mRectangle->setMaterial("HeightmapSimulation");

    sh::Factory::getInstance().setTextureAlias("Heightmap0", mTextures[0]->getName());
    sh::Factory::getInstance().setTextureAlias("Heightmap1", mTextures[1]->getName());

    sh::Factory::getInstance().setSharedParameter("currentFrameOffset", sh::makeProperty<sh::Vector3>(
        new sh::Vector3(mCurrentFrameOffset.x, mCurrentFrameOffset.y, 0)));
    sh::Factory::getInstance().setSharedParameter("previousFrameOffset", sh::makeProperty<sh::Vector3>(
        new sh::Vector3(mPreviousFrameOffset.x, mPreviousFrameOffset.y, 0)));

    mRenderTargets[2]->update();
}

void RippleSimulation::heightMapToNormalMap()
{
    mRectangle->setMaterial("HeightToNormalMap");

    sh::Factory::getInstance().setTextureAlias("Heightmap2", mTextures[2]->getName());

    mRenderTargets[TEX_NORMAL]->update();
}

void RippleSimulation::swapHeightMaps()
{
    // 0 -> 1 -> 2 to 2 -> 0 ->1
    Ogre::RenderTexture* tmp = mRenderTargets[0];
    Ogre::TexturePtr tmp2 = mTextures[0];

    mRenderTargets[0] = mRenderTargets[1];
    mTextures[0] = mTextures[1];

    mRenderTargets[1] = mRenderTargets[2];
    mTextures[1] = mTextures[2];

    mRenderTargets[2] = tmp;
    mTextures[2] = tmp2;
}

void RippleSimulation::addEmitter(const MWWorld::Ptr& ptr, float scale, float force)
{
    Emitter newEmitter;
    newEmitter.mPtr = ptr;
    newEmitter.mScale = scale;
    newEmitter.mForce = force;
    newEmitter.mLastEmitPosition = Ogre::Vector3(0,0,0);
    mEmitters.push_back (newEmitter);
}

void RippleSimulation::removeEmitter (const MWWorld::Ptr& ptr)
{
    for (std::vector<Emitter>::iterator it = mEmitters.begin(); it != mEmitters.end(); ++it)
    {
        if (it->mPtr == ptr)
        {
            mEmitters.erase(it);
            return;
        }
    }
}

void RippleSimulation::updateEmitterPtr (const MWWorld::Ptr& old, const MWWorld::Ptr& ptr)
{
    for (std::vector<Emitter>::iterator it = mEmitters.begin(); it != mEmitters.end(); ++it)
    {
        if (it->mPtr == old)
        {
            it->mPtr = ptr;
            return;
        }
    }
}


}
