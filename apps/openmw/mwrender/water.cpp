#include "water.hpp"

#include <OgreRenderTarget.h>
#include <OgreEntity.h>
#include <OgreMeshManager.h>
#include <OgreHardwarePixelBuffer.h>
#include <OgreRoot.h>

#include "sky.hpp"
#include "renderingmanager.hpp"
#include "ripplesimulation.hpp"
#include "refraction.hpp"

#include <extern/shiny/Main/Factory.hpp>
#include <extern/shiny/Platforms/Ogre/OgreMaterial.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

using namespace Ogre;

namespace MWRender
{

CubeReflection::CubeReflection(Ogre::SceneManager* sceneManager)
    : Reflection(sceneManager)
{
    Ogre::TexturePtr texture = Ogre::TextureManager::getSingleton ().createManual("CubeReflection",
                ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, TEX_TYPE_CUBE_MAP,
                512,512, 0, PF_R8G8B8, TU_RENDERTARGET);

    mCamera = mSceneMgr->createCamera ("CubeCamera");
    mCamera->setNearClipDistance (5);
    mCamera->setFarClipDistance (1000);

    for (int face = 0; face < 6; ++face)
    {
        mRenderTargets[face] = texture->getBuffer (face)->getRenderTarget();
        mRenderTargets[face]->removeAllViewports ();
        Viewport* vp = mRenderTargets[face]->addViewport (mCamera);
        vp->setOverlaysEnabled(false);
        vp->setShadowsEnabled(false);
        vp->setMaterialScheme ("water_reflection");
        mRenderTargets[face]->setAutoUpdated(false);

        /*
        Vector3 lookAt(0,0,0), up(0,0,0), right(0,0,0);
        switch(face)
        {
            case 0:  lookAt.x =-1;  up.y = 1;  right.z = 1;  break;  // +X
            case 1:  lookAt.x = 1;  up.y = 1;  right.z =-1;  break;	 // -X
            case 2:  lookAt.y =-1;  up.z = 1;  right.x = 1;  break;	 // +Y
            case 3:  lookAt.y = 1;  up.z =-1;  right.x = 1;  break;	 // -Y
            case 4:  lookAt.z = 1;  up.y = 1;  right.x =-1;  break;	 // +Z
            case 5:  lookAt.z =-1;  up.y = 1;  right.x =-1;  break;	 // -Z
        }
        Quaternion orient(right, up, lookAt);
        mCamera->setOrientation(orient);
        */
    }
}

CubeReflection::~CubeReflection ()
{
    Ogre::TextureManager::getSingleton ().remove("CubeReflection");
    mSceneMgr->destroyCamera (mCamera);
}

void CubeReflection::update ()
{
    mParentCamera->getParentSceneNode ()->needUpdate ();
    mCamera->setPosition(mParentCamera->getDerivedPosition());
}

// --------------------------------------------------------------------------------------------------------------------------------

PlaneReflection::PlaneReflection(Ogre::SceneManager* sceneManager, SkyManager* sky)
    : Reflection(sceneManager)
    , mSky(sky)
    , mRenderActive(false)
{
    mCamera = mSceneMgr->createCamera ("PlaneReflectionCamera");
    mSceneMgr->addRenderQueueListener(this);

    mTexture = TextureManager::getSingleton().createManual("WaterReflection",
        ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, TEX_TYPE_2D, 512, 512, 0, PF_R8G8B8, TU_RENDERTARGET);

    mRenderTarget = mTexture->getBuffer()->getRenderTarget();
    Viewport* vp = mRenderTarget->addViewport(mCamera);
    vp->setOverlaysEnabled(false);
    vp->setBackgroundColour(ColourValue(0.8f, 0.9f, 1.0f));
    vp->setShadowsEnabled(false);
    vp->setMaterialScheme("water_reflection");
    mRenderTarget->addListener(this);
    mRenderTarget->setActive(true);
    mRenderTarget->setAutoUpdated(true);

    sh::Factory::getInstance ().setTextureAlias ("WaterReflection", mTexture->getName());
}

PlaneReflection::~PlaneReflection ()
{
    mRenderTarget->removeListener (this);
    mSceneMgr->destroyCamera (mCamera);
    mSceneMgr->removeRenderQueueListener(this);
    TextureManager::getSingleton ().remove("WaterReflection");
}

void PlaneReflection::renderQueueStarted (Ogre::uint8 queueGroupId, const Ogre::String &invocation, bool &skipThisInvocation)
{
    // We don't want the sky to get clipped by custom near clip plane (the water plane)
    if (queueGroupId < 20 && mRenderActive)
    {
        mCamera->disableCustomNearClipPlane();
        Root::getSingleton().getRenderSystem()->_setProjectionMatrix(mCamera->getProjectionMatrixRS());
    }
}

void PlaneReflection::renderQueueEnded (Ogre::uint8 queueGroupId, const Ogre::String &invocation, bool &repeatThisInvocation)
{
    if (queueGroupId < 20 && mRenderActive)
    {
        mCamera->enableCustomNearClipPlane(mIsUnderwater ? mErrorPlaneUnderwater : mErrorPlane);
        Root::getSingleton().getRenderSystem()->_setProjectionMatrix(mCamera->getProjectionMatrixRS());
    }
}

void PlaneReflection::preRenderTargetUpdate(const Ogre::RenderTargetEvent& evt)
{
    mParentCamera->getParentSceneNode ()->needUpdate ();
    mCamera->setOrientation(mParentCamera->getDerivedOrientation());
    mCamera->setPosition(mParentCamera->getDerivedPosition());
    mCamera->setNearClipDistance(mParentCamera->getNearClipDistance());
    mCamera->setFarClipDistance(mParentCamera->getFarClipDistance());
    mCamera->setAspectRatio(mParentCamera->getAspectRatio());
    mCamera->setFOVy(mParentCamera->getFOVy());
    mRenderActive = true;

    Vector3 pos = mParentCamera->getRealPosition();
    pos.y = (mWaterPlane).d*2 - pos.y;
    mSky->setSkyPosition(pos);
    mCamera->enableReflection(mWaterPlane);

    // for depth calculation, we want the original viewproj matrix _without_ the custom near clip plane.
    // since all we are interested in is depth, we only need the third row of the matrix.
    Ogre::Matrix4 projMatrix = mCamera->getProjectionMatrixWithRSDepth () * mCamera->getViewMatrix ();
    sh::Vector4* row3 = new sh::Vector4(projMatrix[2][0], projMatrix[2][1], projMatrix[2][2], projMatrix[2][3]);
    sh::Factory::getInstance ().setSharedParameter ("vpRow2Fix", sh::makeProperty<sh::Vector4> (row3));

    // enable clip plane here to take advantage of CPU culling for overwater or underwater objects
    mCamera->enableCustomNearClipPlane(mIsUnderwater ? mErrorPlaneUnderwater : mErrorPlane);
}

void PlaneReflection::postRenderTargetUpdate(const Ogre::RenderTargetEvent& evt)
{
    mSky->resetSkyPosition();
    mCamera->disableReflection();
    mCamera->disableCustomNearClipPlane();
    mRenderActive = false;
}

void PlaneReflection::setHeight (float height)
{
    mWaterPlane = Plane(Ogre::Vector3(0,0,1), height);
    mErrorPlane = Plane(Ogre::Vector3(0,0,1), height - 5);
    mErrorPlaneUnderwater = Plane(Ogre::Vector3(0,0,-1), -height - 5);
}

void PlaneReflection::setActive (bool active)
{
    mRenderTarget->setActive(active);
}

void PlaneReflection::setViewportBackground(Ogre::ColourValue colour)
{
    mRenderTarget->getViewport (0)->setBackgroundColour (colour);
}

void PlaneReflection::setVisibilityMask (int flags)
{
    mRenderTarget->getViewport (0)->setVisibilityMask (flags);
}

// --------------------------------------------------------------------------------------------------------------------------------

Water::Water (Ogre::Camera *camera, RenderingManager* rend) :
    mCamera (camera), mSceneMgr (camera->getSceneManager()),
    mIsUnderwater(false), mVisibilityFlags(0),
    mActive(1), mToggled(1),
    mRendering(rend),
    mWaterTimer(0.f),
    mReflection(NULL),
    mRefraction(NULL),
    mSimulation(NULL),
    mPlayer(0,0)
{
    mSimulation = new RippleSimulation(mSceneMgr);

    mSky = rend->getSkyManager();

    mMaterial = MaterialManager::getSingleton().getByName("Water");

    mTop = 0;

    mIsUnderwater = false;

    mWaterPlane = Plane(Vector3::UNIT_Z, 0);

    MeshManager::getSingleton().createPlane("water", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,  mWaterPlane, CELL_SIZE*5, CELL_SIZE * 5, 10, 10, true, 1, 3,3, Vector3::UNIT_Y);

    mWater = mSceneMgr->createEntity("water");
    mWater->setVisibilityFlags(RV_Water);
    mWater->setCastShadows(false);
    mWater->setRenderQueueGroup(RQG_Alpha);

    mWaterNode = mSceneMgr->getRootSceneNode()->createChildSceneNode();

    mWaterNode->attachObject(mWater);

    applyRTT();
    applyVisibilityMask();

    mWater->setMaterial(mMaterial);

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

    mWaterNode->detachObject(mWater);
    mSceneMgr->destroyEntity(mWater);
    mSceneMgr->destroySceneNode(mWaterNode);

    delete mReflection;
    delete mRefraction;
}

void Water::changeCell(const ESM::Cell* cell)
{
    mTop = cell->mWater;

    setHeight(mTop);

    if(!(cell->mData.mFlags & cell->Interior))
        mWaterNode->setPosition(getSceneNodeCoordinates(cell->mData.mX, cell->mData.mY));
}

void Water::setHeight(const float height)
{
    mTop = height;

    mWaterPlane = Plane(Vector3::UNIT_Z, -height);

    if (mReflection)
        mReflection->setHeight(height);
    if (mRefraction)
        mRefraction->setHeight(height);

    mWaterNode->setPosition(0, 0, height);
    sh::Factory::getInstance ().setSharedParameter ("waterLevel", sh::makeProperty<sh::FloatValue>(new sh::FloatValue(height)));
}

void Water::toggle()
{
    mToggled = !mToggled;
    updateVisible();
}

void
Water::updateUnderwater(bool underwater)
{
    if (!mActive) {
        return;
    }
    mIsUnderwater =
        underwater &&
        mWater->isVisible() &&
        mCamera->getPolygonMode() == Ogre::PM_SOLID;

    if (mReflection)
        mReflection->setUnderwater (mIsUnderwater);
    if (mRefraction)
        mRefraction->setUnderwater (mIsUnderwater);

    updateVisible();
}

Vector3 Water::getSceneNodeCoordinates(int gridX, int gridY)
{
    return Vector3(gridX * CELL_SIZE + (CELL_SIZE / 2), gridY * CELL_SIZE + (CELL_SIZE / 2), mTop);
}

void Water::setViewportBackground(const ColourValue& bg)
{
    if (mReflection)
        mReflection->setViewportBackground(bg);
}

void Water::updateVisible()
{
    mWater->setVisible(mToggled && mActive);
    if (mReflection)
        mReflection->setActive(mToggled && mActive);
    if (mRefraction)
        mRefraction->setActive(mToggled && mActive);
}

void Water::update(float dt, Ogre::Vector3 player)
{
    mWaterTimer += dt;
    sh::Factory::getInstance ().setSharedParameter ("waterTimer", sh::makeProperty<sh::FloatValue>(new sh::FloatValue(mWaterTimer)));

    mRendering->getSkyManager ()->setGlareEnabled (!mIsUnderwater);

    mPlayer = Ogre::Vector2(player.x, player.y);
}

void Water::frameStarted(float dt)
{
    mSimulation->update(dt, mPlayer);

    if (mReflection)
        mReflection->update();
}

void Water::applyRTT()
{
    delete mReflection;
    mReflection = NULL;
    delete mRefraction;
    mRefraction = NULL;

    // Create rendertarget for reflection
    //int rttsize = Settings::Manager::getInt("rtt size", "Water");

    if (Settings::Manager::getBool("shader", "Water"))
    {
        mReflection = new PlaneReflection(mSceneMgr, mSky);
        mReflection->setParentCamera (mCamera);
        mReflection->setHeight(mTop);

        if (Settings::Manager::getBool("refraction", "Water"))
        {
            mRefraction = new Refraction(mCamera);
            mRefraction->setHeight(mTop);
        }
    }

    updateVisible();
}

void Water::applyVisibilityMask()
{
    mVisibilityFlags = RV_Terrain * Settings::Manager::getBool("reflect terrain", "Water")
                        + RV_Statics * Settings::Manager::getBool("reflect statics", "Water")
                        + RV_StaticsSmall * Settings::Manager::getBool("reflect small statics", "Water")
                        + RV_Actors * Settings::Manager::getBool("reflect actors", "Water")
                        + RV_Misc * Settings::Manager::getBool("reflect misc", "Water")
                        + RV_Sky;

    if (mReflection)
        mReflection->setVisibilityMask(mVisibilityFlags);
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
            || it->second == "refraction"
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

void Water::addEmitter (const MWWorld::Ptr& ptr, float scale, float force)
{
    mSimulation->addEmitter (ptr, scale, force);
}

void Water::removeEmitter (const MWWorld::Ptr& ptr)
{
    mSimulation->removeEmitter (ptr);
}

void Water::updateEmitterPtr (const MWWorld::Ptr& old, const MWWorld::Ptr& ptr)
{
    mSimulation->updateEmitterPtr(old, ptr);
}

} // namespace
