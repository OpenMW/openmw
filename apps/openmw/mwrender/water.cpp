#include "water.hpp"
#include <components/settings/settings.hpp>
#include "sky.hpp"
#include "renderingmanager.hpp"

using namespace Ogre;

namespace MWRender
{

Water::Water (Ogre::Camera *camera, SkyManager* sky, const ESM::Cell* cell) :
    mCamera (camera), mViewport (camera->getViewport()), mSceneManager (camera->getSceneManager()),
    mIsUnderwater(false), mVisibilityFlags(0),
    mReflectionTarget(0), mActive(1), mToggled(1),
    mReflectionRenderActive(false)
{
    mSky = sky;

    try
    {
        CompositorManager::getSingleton().setCompositorEnabled(mViewport, "Water", false);
    } catch(...) {}

    mTop = cell->water;

    mIsUnderwater = false;

    mWaterPlane = Plane(Vector3::UNIT_Y, 0);

    MeshManager::getSingleton().createPlane("water", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,  mWaterPlane, CELL_SIZE*5, CELL_SIZE * 5, 10, 10, true, 1, 3,3, Vector3::UNIT_Z);

    mWater = mSceneManager->createEntity("water");
    mWater->setVisibilityFlags(RV_Water);
    mWater->setRenderQueueGroup(RQG_Water);
    mWater->setCastShadows(false);

    mVisibilityFlags = RV_Terrain * Settings::Manager::getBool("reflect terrain", "Water")
                        + RV_Statics * Settings::Manager::getBool("reflect statics", "Water")
                        + RV_StaticsSmall * Settings::Manager::getBool("reflect small statics", "Water")
                        + RV_Actors * Settings::Manager::getBool("reflect actors", "Water")
                        + RV_Misc * Settings::Manager::getBool("reflect misc", "Water")
                        + RV_Sky;

    mWaterNode = mSceneManager->getRootSceneNode()->createChildSceneNode();
    mWaterNode->setPosition(0, mTop, 0);

    mReflectionCamera = mSceneManager->createCamera("ReflectionCamera");

    if(!(cell->data.flags & cell->Interior))
    {
        mWaterNode->setPosition(getSceneNodeCoordinates(cell->data.gridX, cell->data.gridY));
    }
    mWaterNode->attachObject(mWater);

    // Create rendertarget for reflection
    int rttsize = Settings::Manager::getInt("rtt size", "Water");

    TexturePtr tex;
    if (Settings::Manager::getBool("shader", "Water"))
    {
        tex = TextureManager::getSingleton().createManual("WaterReflection",
            ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, TEX_TYPE_2D, rttsize, rttsize, 0, PF_FLOAT16_RGBA, TU_RENDERTARGET);

        RenderTarget* rtt = tex->getBuffer()->getRenderTarget();
        Viewport* vp = rtt->addViewport(mReflectionCamera);
        vp->setOverlaysEnabled(false);
        vp->setBackgroundColour(ColourValue(0.8f, 0.9f, 1.0f));
        vp->setShadowsEnabled(false);
        vp->setVisibilityMask( mVisibilityFlags );
        // use fallback techniques without shadows and without mrt (currently not implemented for sky and terrain)
        //vp->setMaterialScheme("Fallback");
        rtt->addListener(this);
        rtt->setActive(true);

        mReflectionTarget = rtt;
    }

    mCompositorName = RenderingManager::useMRT() ? "Underwater" : "UnderwaterNoMRT";

    createMaterial();
    mWater->setMaterial(mMaterial);

    mUnderwaterEffect = Settings::Manager::getBool("underwater effect", "Water");

    mSceneManager->addRenderQueueListener(this);


    // ----------------------------------------------------------------------------------------------
    // ---------------------------------- reflection debug overlay ----------------------------------
    // ----------------------------------------------------------------------------------------------
    /*
    if (Settings::Manager::getBool("shader", "Water"))
    {
        OverlayManager& mgr = OverlayManager::getSingleton();
        Overlay* overlay;
        // destroy if already exists
        if (overlay = mgr.getByName("ReflectionDebugOverlay"))
            mgr.destroy(overlay);

        overlay = mgr.create("ReflectionDebugOverlay");

        if (MaterialManager::getSingleton().resourceExists("Ogre/ReflectionDebugTexture"))
            MaterialManager::getSingleton().remove("Ogre/ReflectionDebugTexture");
        MaterialPtr debugMat = MaterialManager::getSingleton().create(
            "Ogre/ReflectionDebugTexture",
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
                    mgr.getOverlayElement("Ogre/ReflectionDebugTexPanel"
                )))
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

    mWaterNode->detachObject(mWater);
    mSceneManager->destroyEntity(mWater);
    mSceneManager->destroySceneNode(mWaterNode);

    CompositorManager::getSingleton().removeCompositorChain(mViewport);
}

void Water::changeCell(const ESM::Cell* cell)
{
    mTop = cell->water;

    if(!(cell->data.flags & cell->Interior))
        mWaterNode->setPosition(getSceneNodeCoordinates(cell->data.gridX, cell->data.gridY));
    else
        setHeight(mTop);
}

void Water::setHeight(const float height)
{
    mTop = height;
    mWaterPlane = Plane(Vector3::UNIT_Y, height);
    mWaterNode->setPosition(0, height, 0);
}

void Water::toggle()
{
    mToggled = !mToggled;
    updateVisible();
}

void Water::checkUnderwater(float y)
{
    if (!mActive) return;
    if ((mIsUnderwater && y > mTop) || !mWater->isVisible() || mCamera->getPolygonMode() != Ogre::PM_SOLID)
    {
        CompositorManager::getSingleton().setCompositorEnabled(mViewport, mCompositorName, false);

        // tell the shader we are not underwater
        Ogre::Pass* pass = mMaterial->getTechnique(0)->getPass(0);
        if (pass->hasFragmentProgram() && pass->getFragmentProgramParameters()->_findNamedConstantDefinition("isUnderwater", false))
            pass->getFragmentProgramParameters()->setNamedConstant("isUnderwater", Real(0));

        mWater->setRenderQueueGroup(RQG_Water);

        mIsUnderwater = false;
    }

    if (!mIsUnderwater && y < mTop && mWater->isVisible() && mCamera->getPolygonMode() == Ogre::PM_SOLID)
    {
        if (mUnderwaterEffect)
            CompositorManager::getSingleton().setCompositorEnabled(mViewport, mCompositorName, true);

        // tell the shader we are underwater
        Ogre::Pass* pass = mMaterial->getTechnique(0)->getPass(0);
        if (pass->hasFragmentProgram() && pass->getFragmentProgramParameters()->_findNamedConstantDefinition("isUnderwater", false))
            pass->getFragmentProgramParameters()->setNamedConstant("isUnderwater", Real(1));

        mWater->setRenderQueueGroup(RQG_UnderWater);

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

        /// \todo For some reason this camera is delayed for 1 frame, which causes ugly sky reflection behaviour..
        /// to circumvent this we just scale the sky up, so it's not that noticable
        Vector3 pos = mCamera->getRealPosition();
        pos.y = mTop*2 - pos.y;
        mSky->setSkyPosition(pos);
        mSky->scaleSky(mCamera->getFarClipDistance() / 5000.f);
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
    mMaterial = MaterialManager::getSingleton().getByName("Water");

    // these have to be set in code
    std::string textureNames[32];
    for (int i=0; i<32; ++i)
    {
        textureNames[i] = "textures\\water\\water" + StringConverter::toString(i, 2, '0') + ".dds";
    }
    mMaterial->getTechnique(1)->getPass(0)->getTextureUnitState(0)->setAnimatedTextureName(textureNames, 32, 2);

    // use technique without shaders if reflection is disabled
    if (mReflectionTarget == 0)
        mMaterial->removeTechnique(0);

    if (Settings::Manager::getBool("shader", "Water"))
    {
        CompositorInstance* compositor = CompositorManager::getSingleton().getCompositorChain(mViewport)->getCompositor("gbuffer");

        TexturePtr colorTexture = compositor->getTextureInstance("mrt_output", 0);
        TextureUnitState* tus = mMaterial->getTechnique(0)->getPass(0)->getTextureUnitState("refractionMap");
        if (tus != 0)
            tus->setTexture(colorTexture);

        TexturePtr depthTexture = compositor->getTextureInstance("mrt_output", 1);
        tus = mMaterial->getTechnique(0)->getPass(0)->getTextureUnitState("depthMap");
        if (tus != 0)
            tus->setTexture(depthTexture);
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
        mReflectionTarget->setActive(mToggled && mActive && !mIsUnderwater);
}

void Water::renderQueueStarted (Ogre::uint8 queueGroupId, const Ogre::String &invocation, bool &skipThisInvocation)
{
    // We don't want the sky to get clipped by custom near clip plane (the water plane)
    if (queueGroupId < 20 && mReflectionRenderActive)
    {
        mReflectionCamera->disableCustomNearClipPlane();
        Root::getSingleton().getRenderSystem()->_setProjectionMatrix(mReflectionCamera->getProjectionMatrixRS());
    }
}

void Water::renderQueueEnded (Ogre::uint8 queueGroupId, const Ogre::String &invocation, bool &repeatThisInvocation)
{
    if (queueGroupId < 20 && mReflectionRenderActive)
    {
        mReflectionCamera->enableCustomNearClipPlane(mWaterPlane);
        Root::getSingleton().getRenderSystem()->_setProjectionMatrix(mReflectionCamera->getProjectionMatrixRS());
    }
}

void Water::update()
{
}

} // namespace
