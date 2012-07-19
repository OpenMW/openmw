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

using namespace Ogre;

namespace MWRender
{

Water::Water (Ogre::Camera *camera, RenderingManager* rend, const ESM::Cell* cell) :
    mCamera (camera), mSceneManager (camera->getSceneManager()),
    mIsUnderwater(false), mVisibilityFlags(0),
    mReflectionTarget(0), mActive(1), mToggled(1),
    mReflectionRenderActive(false), mRendering(rend),
    mOldFarClip(0),
    mWaterTimer(0.f)
{
    mSky = rend->getSkyManager();

    sh::Factory::getInstance ().setSharedParameter ("windDir_windSpeed", sh::makeProperty<sh::Vector3>(new sh::Vector3(0.5, -0.8, 0.2)));
    sh::Factory::getInstance ().setSharedParameter ("waterTimer", sh::makeProperty<sh::FloatValue>(new sh::FloatValue(0)));
    sh::Factory::getInstance ().setSharedParameter ("waterSunFade_sunHeight", sh::makeProperty<sh::Vector2>(new sh::Vector2(1, 0.6)));

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


    createMaterial();
    mWater->setMaterial(mMaterial);

    mUnderwaterEffect = Settings::Manager::getBool("underwater effect", "Water");

    Ogre::Entity* underwaterDome = mSceneManager->createEntity ("underwater_dome.mesh");
    underwaterDome->setRenderQueueGroup (RQG_UnderWater);
    mUnderwaterDome = mSceneManager->getRootSceneNode ()->createChildSceneNode ();
    mUnderwaterDome->attachObject (underwaterDome);
    mUnderwaterDome->setScale(100,100,100);
    mUnderwaterDome->setVisible(false);

    mSceneManager->addRenderQueueListener(this);

    assignTextures();

    setHeight(mTop);


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
        TextureUnitState *t = debugMat->getTechnique(0)->getPass(0)->createTextureUnitState(mReflectionTexture->getName());

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
        mRendering->getCompositors()->setCompositorEnabled(mCompositorName, false);
        return;
    }

    if ((mIsUnderwater && y > mTop) || !mWater->isVisible() || mCamera->getPolygonMode() != Ogre::PM_SOLID)
    {
        //mRendering->getCompositors()->setCompositorEnabled(mCompositorName, false);

        // tell the shader we are not underwater

/*
        Ogre::Pass* pass = mMaterial->getTechnique(0)->getPass(0);
        if (pass->hasFragmentProgram() && pass->getFragmentProgramParameters()->_findNamedConstantDefinition("isUnderwater", false))
            pass->getFragmentProgramParameters()->setNamedConstant("isUnderwater", Real(0));
*/
        mWater->setRenderQueueGroup(RQG_Water);

        mIsUnderwater = false;
    }

    if (!mIsUnderwater && y < mTop && mWater->isVisible() && mCamera->getPolygonMode() == Ogre::PM_SOLID)
    {
        //if (mUnderwaterEffect)
            //mRendering->getCompositors()->setCompositorEnabled(mCompositorName, true);

        // tell the shader we are underwater
/*
        Ogre::Pass* pass = mMaterial->getTechnique(0)->getPass(0);
        if (pass->hasFragmentProgram() && pass->getFragmentProgramParameters()->_findNamedConstantDefinition("isUnderwater", false))
            pass->getFragmentProgramParameters()->setNamedConstant("isUnderwater", Real(1));
*/
        //mWater->setRenderQueueGroup(RQG_UnderWater);

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

void Water::createMaterial()
{
    if (mReflectionTarget == 0)
    {
        mMaterial = MaterialManager::getSingleton().getByName("Water_Fallback");
    }
    else
    {
        mMaterial = MaterialManager::getSingleton().getByName("Water");
        sh::Factory::getInstance ().setTextureAlias ("WaterReflection", mReflectionTexture->getName());
    }

    // these have to be set in code
    std::string textureNames[32];
    for (int i=0; i<32; ++i)
    {
        textureNames[i] = "textures\\water\\water" + StringConverter::toString(i, 2, '0') + ".dds";
    }

    if (mReflectionTarget == 0)
        mMaterial->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setAnimatedTextureName(textureNames, 32, 2);
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
}

void Water::renderQueueEnded (Ogre::uint8 queueGroupId, const Ogre::String &invocation, bool &repeatThisInvocation)
{
    if (queueGroupId < 20 && mReflectionRenderActive)
    {
        mReflectionCamera->setFarClipDistance (mOldFarClip);
        if (!mIsUnderwater)
            mReflectionCamera->enableCustomNearClipPlane(mWaterPlane);
        Root::getSingleton().getRenderSystem()->_setProjectionMatrix(mReflectionCamera->getProjectionMatrixRS());
    }
}

void Water::update(float dt)
{
    Ogre::Vector3 pos = mCamera->getDerivedPosition ();
    pos.y = -mWaterPlane.d;
    mUnderwaterDome->setPosition (pos);

    mWaterTimer += dt;
    sh::Factory::getInstance ().setSharedParameter ("waterTimer", sh::makeProperty<sh::FloatValue>(new sh::FloatValue(mWaterTimer)));
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
    }

    mCompositorName = RenderingManager::useMRT() ? "Underwater" : "UnderwaterNoMRT";
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
        createMaterial();
        mWater->setMaterial(mMaterial);
        assignTextures();
    }
    if (applyVisMask)
        applyVisibilityMask();
}

} // namespace
