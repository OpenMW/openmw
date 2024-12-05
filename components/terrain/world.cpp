#include "world.hpp"

#include <osg/Camera>
#include <osg/Group>

#include <components/resource/resourcesystem.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/settings/values.hpp>

#include "chunkmanager.hpp"
#include "compositemaprenderer.hpp"
#include "heightcull.hpp"
#include "storage.hpp"
#include "texturemanager.hpp"

namespace Terrain
{

    World::World(osg::Group* parent, osg::Group* compileRoot, Resource::ResourceSystem* resourceSystem,
        Storage* storage, unsigned int nodeMask, unsigned int preCompileMask, unsigned int borderMask,
        ESM::RefId worldspace, double expiryDelay)
        : mStorage(storage)
        , mParent(parent)
        , mResourceSystem(resourceSystem)
        , mBorderVisible(false)
        , mWorldspace(worldspace)
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

        mTextureManager = std::make_unique<TextureManager>(mResourceSystem->getSceneManager(), expiryDelay);
        mChunkManager = std::make_unique<ChunkManager>(mStorage, mResourceSystem->getSceneManager(),
            mTextureManager.get(), mCompositeMapRenderer, mWorldspace, expiryDelay);
        mChunkManager->setNodeMask(nodeMask);
        mCellBorder
            = std::make_unique<CellBorder>(this, mTerrainRoot.get(), borderMask, mResourceSystem->getSceneManager());

        mResourceSystem->addResourceManager(mChunkManager.get());
        mResourceSystem->addResourceManager(mTextureManager.get());
    }

    World::World(osg::Group* parent, Storage* storage, unsigned int nodeMask, ESM::RefId worldspace)
        : mStorage(storage)
        , mParent(parent)
        , mCompositeMapCamera(nullptr)
        , mCompositeMapRenderer(nullptr)
        , mResourceSystem(nullptr)
        , mTextureManager(nullptr)
        , mChunkManager(nullptr)
        , mCellBorder(nullptr)
        , mBorderVisible(false)
        , mWorldspace(worldspace)
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

    void World::setBordersVisible(bool visible)
    {
        mBorderVisible = visible;

        if (visible)
        {
            for (std::set<std::pair<int, int>>::iterator it = mLoadedCells.begin(); it != mLoadedCells.end(); ++it)
                mCellBorder->createCellBorderGeometry(it->first, it->second);
        }
        else
            mCellBorder->destroyCellBorderGeometry();
    }

    void World::loadCell(int x, int y)
    {
        if (mBorderVisible)
            mCellBorder->createCellBorderGeometry(x, y);

        mLoadedCells.insert(std::pair<int, int>(x, y));
    }

    void World::unloadCell(int x, int y)
    {
        if (mBorderVisible)
            mCellBorder->destroyCellBorderGeometry(x, y);

        mLoadedCells.erase(std::pair<int, int>(x, y));
    }

    void World::setTargetFrameRate(float rate)
    {
        mCompositeMapRenderer->setTargetFrameRate(rate);
    }

    float World::getHeightAt(const osg::Vec3f& worldPos)
    {
        return mStorage->getHeightAt(worldPos, mWorldspace);
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

    void World::enableHeightCullCallback(bool enable)
    {
        if (enable)
            mHeightCullCallback = new HeightCullCallback;
        else
            mHeightCullCallback = nullptr;
    }

    osg::Callback* World::getHeightCullCallback(float highz, unsigned int mask)
    {
        if (!mHeightCullCallback || mTerrainRoot->getNumChildren() == 0)
            return nullptr;

        mHeightCullCallback->setHighZ(highz);
        mHeightCullCallback->setCullMask(mask);
        return mHeightCullCallback;
    }

}
