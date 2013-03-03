#include "refraction.hpp"

#include <OgreCamera.h>
#include <OgreTextureManager.h>
#include <OgreSceneManager.h>
#include <OgreHardwarePixelBuffer.h>
#include <OgreRenderTarget.h>
#include <OgreViewport.h>
#include <OgreRoot.h>

#include <extern/shiny/Main/Factory.hpp>

#include "renderconst.hpp"

namespace MWRender
{

    Refraction::Refraction(Ogre::Camera *parentCamera)
        : mParentCamera(parentCamera)
        , mRenderActive(false)
        , mIsUnderwater(false)
    {
        mCamera = mParentCamera->getSceneManager()->createCamera("RefractionCamera");

        mParentCamera->getSceneManager()->addRenderQueueListener(this);

        Ogre::TexturePtr texture = Ogre::TextureManager::getSingleton().createManual("WaterRefraction",
            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, Ogre::TEX_TYPE_2D, 512, 512, 0, Ogre::PF_R8G8B8, Ogre::TU_RENDERTARGET);

        mRenderTarget = texture->getBuffer()->getRenderTarget();
        Ogre::Viewport* vp = mRenderTarget->addViewport(mCamera);
        vp->setOverlaysEnabled(false);
        vp->setShadowsEnabled(false);
        vp->setVisibilityMask(RV_Actors + RV_Misc + RV_Statics + RV_StaticsSmall + RV_Terrain + RV_Sky);
        vp->setMaterialScheme("water_refraction");
        vp->setBackgroundColour (Ogre::ColourValue(0.18039, 0.23137, 0.25490));
        mRenderTarget->setAutoUpdated(true);
        mRenderTarget->addListener(this);
    }

    Refraction::~Refraction()
    {
        mRenderTarget->removeListener(this);
        Ogre::TextureManager::getSingleton().remove("WaterRefraction");
        mParentCamera->getSceneManager()->destroyCamera(mCamera);
        mParentCamera->getSceneManager()->removeRenderQueueListener(this);
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

        // for depth calculation, we want the original viewproj matrix _without_ the custom near clip plane.
        // since all we are interested in is depth, we only need the third row of the matrix.
        Ogre::Matrix4 projMatrix = mCamera->getProjectionMatrixWithRSDepth () * mCamera->getViewMatrix ();
        sh::Vector4* row3 = new sh::Vector4(projMatrix[2][0], projMatrix[2][1], projMatrix[2][2], projMatrix[2][3]);
        sh::Factory::getInstance ().setSharedParameter ("vpRow2Fix", sh::makeProperty<sh::Vector4> (row3));

        // enable clip plane here to take advantage of CPU culling for overwater or underwater objects
        mCamera->enableCustomNearClipPlane(mIsUnderwater ? mNearClipPlaneUnderwater : mNearClipPlane);

        mRenderActive = true;
    }

    void Refraction::postRenderTargetUpdate(const Ogre::RenderTargetEvent& evt)
    {
        mCamera->disableCustomNearClipPlane ();
        mRenderActive = false;
    }

    void Refraction::setHeight(float height)
    {
        mNearClipPlane = Ogre::Plane( -Ogre::Vector3(0,0,1), -(height + 5));
        mNearClipPlaneUnderwater = Ogre::Plane( Ogre::Vector3(0,0,1), height - 5);
    }

    void Refraction::renderQueueStarted (Ogre::uint8 queueGroupId, const Ogre::String &invocation, bool &skipThisInvocation)
    {
        // We don't want the sky to get clipped by custom near clip plane (the water plane)
        if (queueGroupId < 20 && mRenderActive)
        {
            mCamera->disableCustomNearClipPlane();
            Ogre::Root::getSingleton().getRenderSystem()->_setProjectionMatrix(mCamera->getProjectionMatrixRS());
        }
    }

    void Refraction::renderQueueEnded (Ogre::uint8 queueGroupId, const Ogre::String &invocation, bool &repeatThisInvocation)
    {
        if (queueGroupId < 20 && mRenderActive)
        {
            mCamera->enableCustomNearClipPlane(mIsUnderwater ? mNearClipPlaneUnderwater : mNearClipPlane);
            Ogre::Root::getSingleton().getRenderSystem()->_setProjectionMatrix(mCamera->getProjectionMatrixRS());
        }
    }

    void Refraction::setActive(bool active)
    {
        mRenderTarget->setActive(active);
    }

}
