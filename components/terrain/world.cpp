#include "world.hpp"

#include <osg/Group>
#include <osg/Camera>

#include <components/resource/resourcesystem.hpp>

#include "storage.hpp"
#include "texturemanager.hpp"
#include "chunkmanager.hpp"
#include "compositemaprenderer.hpp"

namespace Terrain
{

World::World(osg::Group* parent, osg::Group* compileRoot, Resource::ResourceSystem* resourceSystem, Storage* storage, unsigned int nodeMask, unsigned int preCompileMask, unsigned int borderMask)
    : mStorage(storage)
    , mParent(parent)
    , mResourceSystem(resourceSystem)
    , mBorderVisible(false)
    , mHeightCullCallback(new HeightCullCallback)
{
    mTerrainRoot = new osg::Group;
    mTerrainRoot->setNodeMask(nodeMask);
    mTerrainRoot->setName("Terrain Root");

    osg::ref_ptr<osg::Camera> compositeCam = new osg::Camera;
    compositeCam->setRenderOrder(osg::Camera::PRE_RENDER, -1);
    compositeCam->setProjectionMatrix(osg::Matrix::identity());
    compositeCam->setViewMatrix(osg::Matrix::identity());
    compositeCam->setReferenceFrame(osg::Camera::ABSOLUTE_RF);
    compositeCam->setClearMask(0);
    compositeCam->setNodeMask(preCompileMask);
    mCompositeMapCamera = compositeCam;

    compileRoot->addChild(compositeCam);

    mCompositeMapRenderer = new CompositeMapRenderer;
    compositeCam->addChild(mCompositeMapRenderer);

    mParent->addChild(mTerrainRoot);

    mTextureManager.reset(new TextureManager(mResourceSystem->getSceneManager()));
    mChunkManager.reset(new ChunkManager(mStorage, mResourceSystem->getSceneManager(), mTextureManager.get(), mCompositeMapRenderer));
    mChunkManager->setNodeMask(nodeMask);
    mCellBorder.reset(new CellBorder(this,mTerrainRoot.get(),borderMask));

    mResourceSystem->addResourceManager(mChunkManager.get());
    mResourceSystem->addResourceManager(mTextureManager.get());
}

World::World(osg::Group* parent, Storage* storage, unsigned int nodeMask)
    : mStorage(storage)
    , mParent(parent)
    , mCompositeMapCamera(nullptr)
    , mCompositeMapRenderer(nullptr)
    , mResourceSystem(nullptr)
    , mTextureManager(nullptr)
    , mChunkManager(nullptr)
    , mCellBorder(nullptr)
    , mBorderVisible(false)
    , mHeightCullCallback(nullptr)
{
    mTerrainRoot = new osg::Group;
    mTerrainRoot->setNodeMask(nodeMask);

    mParent->addChild(mTerrainRoot);
}

World::~World()
{
    if (mResourceSystem && mChunkManager)
        mResourceSystem->removeResourceManager(mChunkManager.get());
    if (mResourceSystem && mTextureManager)
        mResourceSystem->removeResourceManager(mTextureManager.get());

    mParent->removeChild(mTerrainRoot);

    if (mCompositeMapCamera && mCompositeMapRenderer)
    {
        mCompositeMapCamera->removeChild(mCompositeMapRenderer);
        mCompositeMapCamera->getParent(0)->removeChild(mCompositeMapCamera);
    }
}

void World::setWorkQueue(SceneUtil::WorkQueue* workQueue)
{
    mCompositeMapRenderer->setWorkQueue(workQueue);
}

void World::setBordersVisible(bool visible)
{
    mBorderVisible = visible;

    if (visible)
    {
        for (std::set<std::pair<int,int>>::iterator it = mLoadedCells.begin(); it != mLoadedCells.end(); ++it)
            mCellBorder->createCellBorderGeometry(it->first,it->second);
    }
    else
        mCellBorder->destroyCellBorderGeometry();
}

void World::loadCell(int x, int y)
{
    if (mBorderVisible)
        mCellBorder->createCellBorderGeometry(x,y);

    mLoadedCells.insert(std::pair<int,int>(x,y));
}

void World::unloadCell(int x, int y)
{
    if (mBorderVisible)
        mCellBorder->destroyCellBorderGeometry(x,y);

    mLoadedCells.erase(std::pair<int,int>(x,y));
}

void World::setTargetFrameRate(float rate)
{
    mCompositeMapRenderer->setTargetFrameRate(rate);
}

float World::getHeightAt(const osg::Vec3f &worldPos)
{
    return mStorage->getHeightAt(worldPos);
}

void World::updateTextureFiltering()
{
    if (mTextureManager)
        mTextureManager->updateTextureFiltering();
}

void World::clearAssociatedCaches()
{
    if (mChunkManager)
        mChunkManager->clearCache();
}

osg::Callback* World::getHeightCullCallback(float highz, unsigned int mask)
{
    if (!mHeightCullCallback) return nullptr;

    mHeightCullCallback->setHighZ(highz);
    mHeightCullCallback->setCullMask(mask);
    return mHeightCullCallback;
}

}
