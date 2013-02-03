#include "refraction.hpp"

#include <OgreCamera.h>
#include <OgreTextureManager.h>
#include <OgreSceneManager.h>
#include <OgreHardwarePixelBuffer.h>
#include <OgreRenderTarget.h>
#include <OgreViewport.h>

#include "renderconst.hpp"

namespace MWRender
{

    Refraction::Refraction(Ogre::Camera *parentCamera)
        : mParentCamera(parentCamera)
    {
        mCamera = mParentCamera->getSceneManager()->createCamera("RefractionCamera");

        Ogre::TexturePtr texture = Ogre::TextureManager::getSingleton().createManual("WaterRefraction",
            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, Ogre::TEX_TYPE_2D, 512, 512, 0, Ogre::PF_R8G8B8, Ogre::TU_RENDERTARGET);

        mRenderTarget = texture->getBuffer()->getRenderTarget();
        Ogre::Viewport* vp = mRenderTarget->addViewport(mCamera);
        vp->setOverlaysEnabled(false);
        vp->setShadowsEnabled(false);
        vp->setVisibilityMask(RV_Actors + RV_Misc + RV_Statics + RV_StaticsSmall + RV_Terrain);
        vp->setMaterialScheme("water_reflection");
        mRenderTarget->setAutoUpdated(true);
        mRenderTarget->addListener(this);
    }

    Refraction::~Refraction()
    {
        mRenderTarget->removeListener(this);
        Ogre::TextureManager::getSingleton().remove("WaterRefraction");
        mParentCamera->getSceneManager()->destroyCamera(mCamera);
    }

    void Refraction::preRenderTargetUpdate(const Ogre::RenderTargetEvent& evt)
    {
        mParentCamera->getParentSceneNode ()->needUpdate ();
        mCamera->setOrientation(mParentCamera->getDerivedOrientation());
        mCamera->setPosition(mParentCamera->getDerivedPosition());
        mCamera->setNearClipDistance(mParentCamera->getNearClipDistance());
        mCamera->setFarClipDistance(mParentCamera->getFarClipDistance());
        mCamera->setAspectRatio(mParentCamera->getAspectRatio());
        mCamera->setFOVy(mParentCamera->getFOVy());

        mCamera->enableCustomNearClipPlane(mNearClipPlane);
    }

    void Refraction::postRenderTargetUpdate(const Ogre::RenderTargetEvent& evt)
    {

    }

    void Refraction::setHeight(float height)
    {
        mNearClipPlane = Ogre::Plane( -Ogre::Vector3(0,1,0), -(height + 5));
    }

}
