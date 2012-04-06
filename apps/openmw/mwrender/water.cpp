#include "water.hpp"
#include <components/settings/settings.hpp>
#include "sky.hpp"

using namespace Ogre;

namespace MWRender
{

Water::Water (Ogre::Camera *camera, SkyManager* sky, const ESM::Cell* cell) :
    mCamera (camera), mViewport (camera->getViewport()), mSceneManager (camera->getSceneManager()),
    mIsUnderwater(false), mReflectDistance(0), mVisibilityFlags(0), mOldCameraFarClip(0),
    mReflectionTarget(0), mActive(1)
{
    mSky = sky;

    try
    {
        CompositorManager::getSingleton().addCompositor(mViewport, "Water", -1);
        CompositorManager::getSingleton().setCompositorEnabled(mViewport, "Water", false);
    } catch(...) {}

    mTop = cell->water;

    mIsUnderwater = false;

    mWaterPlane = Plane(Vector3::UNIT_Y, 0);

    MeshManager::getSingleton().createPlane("water", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,  mWaterPlane, CELL_SIZE*5, CELL_SIZE * 5, 10, 10, true, 1, 3,3, Vector3::UNIT_Z);

    mWater = mSceneManager->createEntity("water");
    mWater->setVisibilityFlags(RV_Water);
    mWater->setRenderQueueGroup(RQG_Water);

    mVisibilityFlags = RV_Terrain * Settings::Manager::getBool("reflect terrain", "Water")
                        + RV_Statics * Settings::Manager::getBool("reflect statics", "Water")
                        + RV_StaticsSmall * Settings::Manager::getBool("reflect small statics", "Water")
                        + RV_Actors * Settings::Manager::getBool("reflect actors", "Water")
                        + RV_Misc * Settings::Manager::getBool("reflect misc", "Water")
                        + RV_Sky;
    mReflectDistance = Settings::Manager::getInt("reflect distance", "Water");

    mWaterNode = mSceneManager->getRootSceneNode()->createChildSceneNode();
    mWaterNode->setPosition(0, mTop, 0);

    if(!(cell->data.flags & cell->Interior))
    {
        mWaterNode->setPosition(getSceneNodeCoordinates(cell->data.gridX, cell->data.gridY));
    }
    mWaterNode->attachObject(mWater);

    // Create rendertarget for reflection
    int rttsize = Settings::Manager::getInt("rtt size", "Water");

    if (Settings::Manager::getBool("shader", "Water"))
    {
        TexturePtr tex = TextureManager::getSingleton().createManual("WaterReflection",
            ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, TEX_TYPE_2D, rttsize, rttsize, 0, PF_R8G8B8, TU_RENDERTARGET);

        RenderTarget* rtt = tex->getBuffer()->getRenderTarget();
        Viewport* vp = rtt->addViewport(mCamera);
        vp->setOverlaysEnabled(false);
        vp->setBackgroundColour(ColourValue(0.8f, 0.9f, 1.0f));
        vp->setShadowsEnabled(false);
        vp->setVisibilityMask( mVisibilityFlags );
        rtt->addListener(this);
        rtt->setActive(true);

        mReflectionTarget = rtt;
    }

    createMaterial();
    mWater->setMaterial(mMaterial);
}

void Water::setActive(bool active)
{
    mActive = active;
    if (mReflectionTarget) mReflectionTarget->setActive(active);
    mWater->setVisible(active);
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
    mWaterNode->setPosition(0, height, 0);
}

void Water::toggle()
{
    if (mActive)
        mWater->setVisible(!mWater->getVisible());
}

void Water::checkUnderwater(float y)
{
    if (!mActive) return;
    if ((mIsUnderwater && y > mTop) || !mWater->isVisible() || mCamera->getPolygonMode() != Ogre::PM_SOLID)
    {
        try {
            CompositorManager::getSingleton().setCompositorEnabled(mViewport, "Water", false);
        } catch(...) {}

        // tell the shader we are not underwater
        Ogre::Pass* pass = mMaterial->getTechnique(0)->getPass(0);
        if (pass->hasFragmentProgram() && pass->getFragmentProgramParameters()->_findNamedConstantDefinition("isUnderwater", false))
            pass->getFragmentProgramParameters()->setNamedConstant("isUnderwater", Real(0));

        if (mReflectionTarget)
            mReflectionTarget->setActive(mActive);

        mIsUnderwater = false;
    } 

    if (!mIsUnderwater && y < mTop && mWater->isVisible() && mCamera->getPolygonMode() == Ogre::PM_SOLID)
    {
        try {
            CompositorManager::getSingleton().setCompositorEnabled(mViewport, "Water", true);
        } catch(...) {}

        // tell the shader we are underwater
        Ogre::Pass* pass = mMaterial->getTechnique(0)->getPass(0);
        if (pass->hasFragmentProgram() && pass->getFragmentProgramParameters()->_findNamedConstantDefinition("isUnderwater", false))
            pass->getFragmentProgramParameters()->setNamedConstant("isUnderwater", Real(1));

        if (mReflectionTarget)
            mReflectionTarget->setActive(false);

        mIsUnderwater = true;
    }
}

Vector3 Water::getSceneNodeCoordinates(int gridX, int gridY)
{
    return Vector3(gridX * CELL_SIZE + (CELL_SIZE / 2), mTop, -gridY * CELL_SIZE - (CELL_SIZE / 2));
}

void Water::preRenderTargetUpdate(const RenderTargetEvent& evt)
{
    //mOldCameraFarClip = mCamera->getFarClipDistance();
    //if (mReflectDistance != 0)
    //    mCamera->setFarClipDistance(mReflectDistance);

    if (evt.source == mReflectionTarget)
    {
        mWater->setVisible(false);

        // Some messy code to get the skybox to show up at all
        // The problem here is that it gets clipped by the water plane
        // Therefore scale it up a bit
        Vector3 pos = mCamera->getRealPosition();
        pos.y = mTop*2 - pos.y;
        mSky->setSkyPosition(pos);
        mSky->scaleSky(mCamera->getFarClipDistance() / 1000.f);

        mCamera->enableCustomNearClipPlane(Plane(Vector3::UNIT_Y, mTop));
        mCamera->enableReflection(Plane(Vector3::UNIT_Y, mTop));
    }
}

void Water::postRenderTargetUpdate(const RenderTargetEvent& evt)
{
    mWater->setVisible(true);

    //mCamera->setFarClipDistance(mOldCameraFarClip);

    if (evt.source == mReflectionTarget)
    {
        mSky->resetSkyPosition();
        mSky->scaleSky(1);
        mCamera->disableReflection();
        mCamera->disableCustomNearClipPlane();
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

} // namespace
