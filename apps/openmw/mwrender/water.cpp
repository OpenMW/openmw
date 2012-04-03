#include "water.hpp"
#include <components/settings/settings.hpp>

using namespace Ogre;

namespace MWRender
{

Water::Water (Ogre::Camera *camera, const ESM::Cell* cell) :
    mCamera (camera), mViewport (camera->getViewport()), mSceneManager (camera->getSceneManager()),
    mIsUnderwater(false), mReflectDistance(0), mVisibilityFlags(0), mOldCameraFarClip(0),
    mReflectionTarget(0), mRefractionTarget(0), mActive(1)
{
    try
    {
        CompositorManager::getSingleton().addCompositor(mViewport, "Water", -1);
        CompositorManager::getSingleton().setCompositorEnabled(mViewport, "Water", false);
    } catch(...) {}

    mTop = cell->water;

    mIsUnderwater = false;

    mWaterPlane = Plane(Vector3::UNIT_Y, 0);

    MeshManager::getSingleton().createPlane("water", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,  mWaterPlane, CELL_SIZE*5, CELL_SIZE * 5, 10, 10, true, 1, 3,5, Vector3::UNIT_Z);

    mWater = mSceneManager->createEntity("water");
    mWater->setVisibilityFlags(RV_Water);
    mWater->setMaterialName("Examples/Water0");

    mVisibilityFlags = RV_Terrain * Settings::Manager::getBool("reflect terrain", "Water")
                        + RV_Statics * Settings::Manager::getBool("reflect statics", "Water")
                        + RV_StaticsSmall * Settings::Manager::getBool("reflect small statics", "Water")
                        + RV_Actors * Settings::Manager::getBool("reflect actors", "Water")
                        + RV_Misc * Settings::Manager::getBool("reflect misc", "Water");
    mReflectDistance = Settings::Manager::getInt("reflect distance", "Water");

    mWaterNode = mSceneManager->getRootSceneNode()->createChildSceneNode();
    mWaterNode->setPosition(0, mTop, 0);

    if(!(cell->data.flags & cell->Interior))
    {
        mWaterNode->setPosition(getSceneNodeCoordinates(cell->data.gridX, cell->data.gridY));
    }
    mWaterNode->attachObject(mWater);

    // Create rendertargets for reflection and refraction
    int rttsize = Settings::Manager::getInt("rtt size", "Water");
	for (unsigned int i = 0; i < 2; ++i)
	{
		if (i==0 && !Settings::Manager::getBool("reflection", "Water")) continue;
		if (i==1 && !Settings::Manager::getBool("refraction", "Water")) continue;

		TexturePtr tex = TextureManager::getSingleton().createManual(i == 0 ? "WaterReflection" : "WaterRefraction",
			ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, TEX_TYPE_2D, rttsize, rttsize, 0, PF_R8G8B8, TU_RENDERTARGET);

		RenderTarget* rtt = tex->getBuffer()->getRenderTarget();
		Viewport* vp = rtt->addViewport(mCamera);
		vp->setOverlaysEnabled(false);
		vp->setBackgroundColour(ColourValue(0.8f, 0.9f, 1.0f));
		vp->setShadowsEnabled(false);
		vp->setVisibilityMask( (i == 0) ? mVisibilityFlags : RV_All);
		rtt->addListener(this);
        rtt->setActive(true);

		if (i == 0) mReflectionTarget = rtt;
		else mRefractionTarget = rtt;
	}
}

void Water::setActive(bool active)
{
    mActive = active;
    if (mReflectionTarget) mReflectionTarget->setActive(active);
    if (mRefractionTarget) mRefractionTarget->setActive(active);
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
        mIsUnderwater = false;
    } 

    if (!mIsUnderwater && y < mTop && mWater->isVisible() && mCamera->getPolygonMode() == Ogre::PM_SOLID)
    {
        try {
            CompositorManager::getSingleton().setCompositorEnabled(mViewport, "Water", true);
        } catch(...) {}
        mIsUnderwater = true;
    }
}

Vector3 Water::getSceneNodeCoordinates(int gridX, int gridY)
{
    return Vector3(gridX * CELL_SIZE + (CELL_SIZE / 2), mTop, -gridY * CELL_SIZE - (CELL_SIZE / 2));
}

void Water::preRenderTargetUpdate(const RenderTargetEvent& evt)
{
    mWater->setVisible(false);

    mOldCameraFarClip = mCamera->getFarClipDistance();
    if (mReflectDistance != 0)
        mCamera->setFarClipDistance(mReflectDistance);

	if (evt.source == mReflectionTarget)
	{
		mCamera->enableCustomNearClipPlane(mWaterPlane);
		mCamera->enableReflection(Plane(Vector3::UNIT_Y, mWaterNode->_getDerivedPosition().y));
	}
}

void Water::postRenderTargetUpdate(const RenderTargetEvent& evt)
{
    mWater->setVisible(true);

    mCamera->setFarClipDistance(mOldCameraFarClip);

	if (evt.source == mReflectionTarget)
	{
		mCamera->disableReflection();
		mCamera->disableCustomNearClipPlane();
	}
}

} // namespace
