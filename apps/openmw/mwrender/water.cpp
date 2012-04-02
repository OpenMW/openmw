#include "water.hpp"

namespace MWRender
{

Water::Water (Ogre::Camera *camera, const ESM::Cell* cell) :
    mCamera (camera), mViewport (camera->getViewport()), mSceneManager (camera->getSceneManager()),
    mIsUnderwater(false)
{
    try
    {
        Ogre::CompositorManager::getSingleton().addCompositor(mViewport, "Water", -1);
        Ogre::CompositorManager::getSingleton().setCompositorEnabled(mViewport, "Water", false);
    } catch(...) {}

    mTop = cell->water;

    mIsUnderwater = false;

    mWaterPlane = Ogre::Plane(Ogre::Vector3::UNIT_Y, 0);

    Ogre::MeshManager::getSingleton().createPlane("water", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,  mWaterPlane, CELL_SIZE*5, CELL_SIZE * 5, 10, 10, true, 1, 3,5, Ogre::Vector3::UNIT_Z);

    mWater = mSceneManager->createEntity("water");

    mWater->setMaterialName("Examples/Water0");

    mWaterNode = mSceneManager->getRootSceneNode()->createChildSceneNode();
    mWaterNode->setPosition(0, mTop, 0);

    if(!(cell->data.flags & cell->Interior))
    {
        mWaterNode->setPosition(getSceneNodeCoordinates(cell->data.gridX, cell->data.gridY));
    }
    mWaterNode->attachObject(mWater);
}


Water::~Water()
{
    Ogre::MeshManager::getSingleton().remove("water");

    mWaterNode->detachObject(mWater);
    mSceneManager->destroyEntity(mWater);
    mSceneManager->destroySceneNode(mWaterNode);

    Ogre::CompositorManager::getSingleton().removeCompositorChain(mViewport);
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
    mWater->setVisible(!mWater->getVisible());
}

void Water::checkUnderwater(float y)
{
    if ((mIsUnderwater && y > mTop) || !mWater->isVisible() || mCamera->getPolygonMode() != Ogre::PM_SOLID)
    {
        try {
            Ogre::CompositorManager::getSingleton().setCompositorEnabled(mViewport, "Water", false);
        } catch(...) {}
        mIsUnderwater = false;
    } 

    if (!mIsUnderwater && y < mTop && mWater->isVisible() && mCamera->getPolygonMode() == Ogre::PM_SOLID)
    {
        try {
            Ogre::CompositorManager::getSingleton().setCompositorEnabled(mViewport, "Water", true);
        } catch(...) {}
        mIsUnderwater = true;
    }
}

Ogre::Vector3 Water::getSceneNodeCoordinates(int gridX, int gridY)
{
    return Ogre::Vector3(gridX * CELL_SIZE + (CELL_SIZE / 2), mTop, -gridY * CELL_SIZE - (CELL_SIZE / 2));
}

} // namespace
