#include "water.hpp"

#include <OgreRenderTarget.h>
#include <OgreEntity.h>
#include <OgreMeshManager.h>
#include <OgreHardwarePixelBuffer.h>
#include <OgreCompositorManager.h>
#include <OgreCompositorInstance.h>
#include <OgreCompositorChain.h>
#include <OgreRoot.h>
#include <OgreOverlayManager.h>
#include <OgreOverlayContainer.h>
#include <OgreOverlayElement.h>

#include "sky.hpp"
#include "renderingmanager.hpp"
#include "compositors.hpp"

#include <extern/shiny/Main/Factory.hpp>
#include <extern/shiny/Platforms/Ogre/OgreMaterial.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

using namespace Ogre;

namespace MWRender
{

Water::Water (Ogre::Camera *camera, RenderingManager* rend, const ESM::Cell* cell) :
    mCamera (camera), mSceneManager (camera->getSceneManager()),
    mIsUnderwater(false), mVisibilityFlags(0),
    mReflectionTarget(0), mActive(1), mToggled(1),
    mReflectionRenderActive(false), mRendering(rend),
    mOldFarClip(0), mOldFarClip2(0),
    mWaterTimer(0.f)
{
    mSky = rend->getSkyManager();

    mMaterial = MaterialManager::getSingleton().getByName("Water");

    mTop = cell->water;

    mIsUnderwater = false;

    mWaterPlane = Plane(Vector3::UNIT_Y, 0);

    MeshManager::getSingleton().createPlane("water", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,  mWaterPlane, CELL_SIZE*5, CELL_SIZE * 5, 10, 10, true, 1, 3,3, Vector3::UNIT_Z);

    mWater = mSceneManager->createEntity("water");
    mWater->setVisibilityFlags(RV_Water);
    mWater->setRenderQueueGroup(RQG_Water);
    mWater->setCastShadows(false);

    mWaterNode = mSceneManager->getRootSceneNode()->createChildSceneNode();

    mReflectionCamera = mSceneManager->createCamera("ReflectionCamera");

    if(!(cell->data.flags & cell->Interior))
    {
        mWaterNode->setPosition(getSceneNodeCoordinates(cell->data.gridX, cell->data.gridY));
    }
    mWaterNode->attachObject(mWater);

    applyRTT();
    applyVisibilityMask();

    mWater->setMaterial(mMaterial);

    /*
    Ogre::Entity* underwaterDome = mSceneManager->createEntity ("underwater_dome.mesh");
    underwaterDome->setRenderQueueGroup (RQG_UnderWater);
    mUnderwaterDome = mSceneManager->getRootSceneNode ()->createChildSceneNode ();
    mUnderwaterDome->attachObject (underwaterDome);
    mUnderwaterDome->setScale(10000,10000,10000);
    mUnderwaterDome->setVisible(false);
    underwaterDome->setMaterialName("Underwater_Dome");
    */

    mSceneManager->addRenderQueueListener(this);

    assignTextures();

    setHeight(mTop);

    sh::MaterialInstance* m = sh::Factory::getInstance ().getMaterialInstance ("Water");
    m->setListener (this);


    // ----------------------------------------------------------------------------------------------
    // ---------------------------------- reflection debug overlay ----------------------------------
    // ----------------------------------------------------------------------------------------------
/*
    if (Settings::Manager::getBool("shader", "Water"))
    {
        OverlayManager& mgr = OverlayManager::getSingleton();
        Overlay* overlay;
        // destroy if already exists
        if ((overlay = mgr.getByName("ReflectionDebugOverlay")))
            mgr.destroy(overlay);

        overlay = mgr.create("ReflectionDebugOverlay");

        if (MaterialManager::getSingleton().resourceExists("Ogre/ReflectionDebugTexture"))
            MaterialManager::getSingleton().remove("Ogre/ReflectionDebugTexture");
        MaterialPtr debugMat = MaterialManager::getSingleton().create(
            "Ogre/ReflectionDebugTexture",
            ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

        debugMat->getTechnique(0)->getPass(0)->setLightingEnabled(false);
        debugMat->getTechnique(0)->getPass(0)->createTextureUnitState(mReflectionTexture->getName());

        OverlayContainer* debugPanel;

        // destroy container if exists
        try
        {
            if ((debugPanel =
                static_cast<OverlayContainer*>(
                    mgr.getOverlayElement("Ogre/ReflectionDebugTexPanel"
                ))))
                mgr.destroyOverlayElement(debugPanel);
        }
        catch (Ogre::Exception&) {}

        debugPanel = (OverlayContainer*)
            (OverlayManager::getSingleton().createOverlayElement("Panel", "Ogre/ReflectionDebugTexPanel"));
        debugPanel->_setPosition(0, 0.55);
        debugPanel->_setDimensions(0.3, 0.3);
        debugPanel->setMaterialName(debugMat->getName());
        debugPanel->show();
        overlay->add2D(debugPanel);
        overlay->show();
    }
*/
}

void Water::setActive(bool active)
{
    mActive = active;
    updateVisible();

    sh::Factory::getInstance ().setSharedParameter ("waterEnabled", sh::makeProperty<sh::FloatValue> (new sh::FloatValue(active ? 1.0 : 0.0)));
}

Water::~Water()
{
    MeshManager::getSingleton().remove("water");

    if (mReflectionTarget)
        mReflectionTexture->getBuffer()->getRenderTarget()->removeListener(this);

    mWaterNode->detachObject(mWater);
    mSceneManager->destroyEntity(mWater);
    mSceneManager->destroySceneNode(mWaterNode);
}

void Water::changeCell(const ESM::Cell* cell)
{
    mTop = cell->water;

    setHeight(mTop);

    if(!(cell->data.flags & cell->Interior))
        mWaterNode->setPosition(getSceneNodeCoordinates(cell->data.gridX, cell->data.gridY));
}

void Water::setHeight(const float height)
{
    mTop = height;

    mWaterPlane = Plane(Vector3::UNIT_Y, height);

    // small error due to reflection texture size & reflection distortion
    mErrorPlane = Plane(Vector3::UNIT_Y, height - 5);

    mWaterNode->setPosition(0, height, 0);
    sh::Factory::getInstance ().setSharedParameter ("waterLevel", sh::makeProperty<sh::FloatValue>(new sh::FloatValue(height)));
}

void Water::toggle()
{
    mToggled = !mToggled;
    updateVisible();
}

void Water::checkUnderwater(float y)
{
    if (!mActive)
    {
        return;
    }

    if ((mIsUnderwater && y > mTop) || !mWater->isVisible() || mCamera->getPolygonMode() != Ogre::PM_SOLID)
    {
        mIsUnderwater = false;
    }

    if (!mIsUnderwater && y < mTop && mWater->isVisible() && mCamera->getPolygonMode() == Ogre::PM_SOLID)
    {
        mIsUnderwater = true;
    }

    updateVisible();
}

Vector3 Water::getSceneNodeCoordinates(int gridX, int gridY)
{
    return Vector3(gridX * CELL_SIZE + (CELL_SIZE / 2), mTop, -gridY * CELL_SIZE - (CELL_SIZE / 2));
}

void Water::preRenderTargetUpdate(const RenderTargetEvent& evt)
{
    if (evt.source == mReflectionTarget)
    {
        mReflectionCamera->setOrientation(mCamera->getDerivedOrientation());
        mReflectionCamera->setPosition(mCamera->getDerivedPosition());
        mReflectionCamera->setNearClipDistance(mCamera->getNearClipDistance());
        mReflectionCamera->setFarClipDistance(mCamera->getFarClipDistance());
        mReflectionCamera->setAspectRatio(mCamera->getAspectRatio());
        mReflectionCamera->setFOVy(mCamera->getFOVy());
        mReflectionRenderActive = true;

        /// \todo the reflection render (and probably all renderingmanager-updates) lag behind 1 camera frame for some reason
        Vector3 pos = mCamera->getRealPosition();
        pos.y = mTop*2 - pos.y;
        mSky->setSkyPosition(pos);
        mSky->scaleSky(mCamera->getFarClipDistance() / 50.f);
        mReflectionCamera->enableReflection(mWaterPlane);
    }
}

void Water::postRenderTargetUpdate(const RenderTargetEvent& evt)
{
    if (evt.source == mReflectionTarget)
    {
        mSky->resetSkyPosition();
        mSky->scaleSky(1);
        mReflectionCamera->disableReflection();
        mReflectionCamera->disableCustomNearClipPlane();
        mReflectionRenderActive = false;
    }
}

void Water::assignTextures()
{
    if (Settings::Manager::getBool("shader", "Water"))
    {

        CompositorInstance* compositor = CompositorManager::getSingleton().getCompositorChain(mRendering->getViewport())->getCompositor("gbuffer");

        TexturePtr colorTexture = compositor->getTextureInstance("mrt_output", 0);
        sh::Factory::getInstance ().setTextureAlias ("WaterRefraction", colorTexture->getName());

        TexturePtr depthTexture = compositor->getTextureInstance("mrt_output", 1);
        sh::Factory::getInstance ().setTextureAlias ("SceneDepth", depthTexture->getName());
    }
}

void Water::setViewportBackground(const ColourValue& bg)
{
    if (mReflectionTarget)
        mReflectionTarget->getViewport(0)->setBackgroundColour(bg);
}

void Water::updateVisible()
{
    mWater->setVisible(mToggled && mActive);
    if (mReflectionTarget)
        mReflectionTarget->setActive(mToggled && mActive);
}

void Water::renderQueueStarted (Ogre::uint8 queueGroupId, const Ogre::String &invocation, bool &skipThisInvocation)
{
    // We don't want the sky to get clipped by custom near clip plane (the water plane)
    if (queueGroupId < 20 && mReflectionRenderActive)
    {
        mOldFarClip = mReflectionCamera->getFarClipDistance ();
        mReflectionCamera->disableCustomNearClipPlane();
        mReflectionCamera->setFarClipDistance (1000000000);
        Root::getSingleton().getRenderSystem()->_setProjectionMatrix(mReflectionCamera->getProjectionMatrixRS());
    }
    else if (queueGroupId == RQG_UnderWater)
    {/*
        mOldFarClip2 = mCamera->getFarClipDistance ();
        mCamera->setFarClipDistance (1000000000);
        Root::getSingleton().getRenderSystem()->_setProjectionMatrix(mCamera->getProjectionMatrixRS());
    */}
}

void Water::renderQueueEnded (Ogre::uint8 queueGroupId, const Ogre::String &invocation, bool &repeatThisInvocation)
{
    if (queueGroupId < 20 && mReflectionRenderActive)
    {
        mReflectionCamera->setFarClipDistance (mOldFarClip);
        if (!mIsUnderwater)
            mReflectionCamera->enableCustomNearClipPlane(mErrorPlane);
        Root::getSingleton().getRenderSystem()->_setProjectionMatrix(mReflectionCamera->getProjectionMatrixRS());
    }
    if (queueGroupId == RQG_UnderWater)
    {
        /*
        mCamera->setFarClipDistance (mOldFarClip2);
        Root::getSingleton().getRenderSystem()->_setProjectionMatrix(mCamera->getProjectionMatrixRS());
    */}
}

void Water::update(float dt)
{
    /*
    Ogre::Vector3 pos = mCamera->getDerivedPosition ();
    pos.y = -mWaterPlane.d;
    mUnderwaterDome->setPosition (pos);
    */

    mWaterTimer += dt / 30.0 * MWBase::Environment::get().getWorld()->getTimeScaleFactor();
    sh::Factory::getInstance ().setSharedParameter ("waterTimer", sh::makeProperty<sh::FloatValue>(new sh::FloatValue(mWaterTimer)));

    mRendering->getSkyManager ()->setGlareEnabled (!mIsUnderwater);
}

void Water::applyRTT()
{
    if (mReflectionTarget)
    {
        TextureManager::getSingleton().remove("WaterReflection");
        mReflectionTarget = 0;
    }

    // Create rendertarget for reflection
    int rttsize = Settings::Manager::getInt("rtt size", "Water");

    if (Settings::Manager::getBool("shader", "Water"))
    {
        mReflectionTexture = TextureManager::getSingleton().createManual("WaterReflection",
            ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, TEX_TYPE_2D, rttsize, rttsize, 0, PF_FLOAT16_RGBA, TU_RENDERTARGET);

        RenderTarget* rtt = mReflectionTexture->getBuffer()->getRenderTarget();
        Viewport* vp = rtt->addViewport(mReflectionCamera);
        vp->setOverlaysEnabled(false);
        vp->setBackgroundColour(ColourValue(0.8f, 0.9f, 1.0f));
        vp->setShadowsEnabled(false);
        // use fallback techniques without shadows and without mrt (currently not implemented for sky and terrain)
        vp->setMaterialScheme("water_reflection");
        rtt->addListener(this);
        rtt->setActive(true);

        mReflectionTarget = rtt;

        sh::Factory::getInstance ().setTextureAlias ("WaterReflection", mReflectionTexture->getName());
    }
}

void Water::applyVisibilityMask()
{
    mVisibilityFlags = RV_Terrain * Settings::Manager::getBool("reflect terrain", "Water")
                        + RV_Statics * Settings::Manager::getBool("reflect statics", "Water")
                        + RV_StaticsSmall * Settings::Manager::getBool("reflect small statics", "Water")
                        + RV_Actors * Settings::Manager::getBool("reflect actors", "Water")
                        + RV_Misc * Settings::Manager::getBool("reflect misc", "Water")
                        + RV_Sky;

    if (mReflectionTarget)
    {
        mReflectionTexture->getBuffer()->getRenderTarget()->getViewport(0)->setVisibilityMask(mVisibilityFlags);
    }
}

void Water::processChangedSettings(const Settings::CategorySettingVector& settings)
{
    bool applyRT = false;
    bool applyVisMask = false;
    for (Settings::CategorySettingVector::const_iterator it=settings.begin();
            it != settings.end(); ++it)
    {
        if ( it->first == "Water" && (
               it->second == "shader"
            || it->second == "rtt size"))
            applyRT = true;

        if ( it->first == "Water" && (
               it->second == "reflect actors"
            || it->second == "reflect terrain"
            || it->second == "reflect misc"
            || it->second == "reflect small statics"
            || it->second == "reflect statics"))
            applyVisMask = true;
    }

    if(applyRT)
    {
        applyRTT();
        applyVisibilityMask();
        mWater->setMaterial(mMaterial);
        assignTextures();
    }
    if (applyVisMask)
        applyVisibilityMask();
}

void Water::requestedConfiguration (sh::MaterialInstance* m, const std::string& configuration)
{
}

void Water::createdConfiguration (sh::MaterialInstance* m, const std::string& configuration)
{
    if (configuration == "local_map" || !Settings::Manager::getBool("shader", "Water"))
    {
        // for simple water, set animated texture names
        // these have to be set in code
        std::string textureNames[32];
        for (int i=0; i<32; ++i)
        {
            textureNames[i] = "textures\\water\\water" + StringConverter::toString(i, 2, '0') + ".dds";
        }

        Ogre::Technique* t = static_cast<sh::OgreMaterial*>(m->getMaterial())->getOgreTechniqueForConfiguration(configuration);
        if (t->getPass(0)->getNumTextureUnitStates () == 0)
            return;
        t->getPass(0)->getTextureUnitState(0)->setAnimatedTextureName(textureNames, 32, 2);
        t->getPass(0)->setDepthWriteEnabled (false);
        t->getPass(0)->setSceneBlending (Ogre::SBT_TRANSPARENT_ALPHA);
    }
}

} // namespace
