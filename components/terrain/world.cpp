#include "world.hpp"

#include <osg/Group>
#include <osg/Material>
#include <osg/Camera>

#include <components/resource/resourcesystem.hpp>
#include <components/settings/settings.hpp>

#include <components/sceneutil/occlusionquerynode.hpp>
#include <components/sceneutil/vismask.hpp>

#include "storage.hpp"
#include "texturemanager.hpp"
#include "chunkmanager.hpp"
#include "compositemaprenderer.hpp"

namespace Terrain
{

void World::resetSettings()
{
    mOQNSettings.enable = Settings::Manager::getBool("octree occlusion queries enable", "OcclusionQueries") && Settings::Manager::getBool("terrain OQN enable", "OcclusionQueries");
    mOQNSettings.debugDisplay = Settings::Manager::getBool("debug occlusion queries", "OcclusionQueries");
    mOQNSettings.querypixelcount = Settings::Manager::getInt("visibility threshold", "OcclusionQueries");
    mOQNSettings.queryframecount = Settings::Manager::getInt("queries frame count", "OcclusionQueries");
    mOQNSettings.minOQNSize = Settings::Manager::getFloat("min node size", "OcclusionQueries");
    mOQNSettings.maxOQNCapacity = Settings::Manager::getInt("max node capacity", "OcclusionQueries");
    mOQNSettings.querymargin = Settings::Manager::getFloat("queries margin", "OcclusionQueries");
    mOQNSettings.maxBVHOQLevelCount = Settings::Manager::getInt("max BVH OQ level count", "OcclusionQueries");
    mOQNSettings.securepopdistance = Settings::Manager::getFloat("min pop in distance", "OcclusionQueries");
    ///update current oqns settings
    SceneUtil::SettingsUpdatorVisitor updator(mOQNSettings);
    mTerrainRoot->accept(updator);
}

World::World(osg::Group* parent, osg::Group* compileRoot, Resource::ResourceSystem* resourceSystem, Storage* storage, const SceneUtil::OcclusionQuerySettings& oqsettings)
    : mStorage(storage)
    , mParent(parent)
    , mResourceSystem(resourceSystem)
    , mBorderVisible(false)
    , mOQNSettings(oqsettings)
{
    mTerrainRoot = new osg::Group;
    resetSettings();
    //MY mTerrainRoot->setNodeMask(SceneUtil::Mask_Terrain);
    mTerrainRoot->getOrCreateStateSet()->setRenderingHint(osg::StateSet::OPAQUE_BIN);
    osg::ref_ptr<osg::Material> material (new osg::Material);
    material->setColorMode(osg::Material::AMBIENT_AND_DIFFUSE);
    mTerrainRoot->getOrCreateStateSet()->setAttributeAndModes(material, osg::StateAttribute::ON);

    mTerrainRoot->setName("Terrain Root");

    osg::ref_ptr<osg::Camera> compositeCam = new osg::Camera;
    compositeCam->setRenderOrder(osg::Camera::PRE_RENDER, -1);
    compositeCam->setProjectionMatrix(osg::Matrix::identity());
    compositeCam->setViewMatrix(osg::Matrix::identity());
    compositeCam->setReferenceFrame(osg::Camera::ABSOLUTE_RF);
    compositeCam->setClearMask(SceneUtil::Mask_Disabled);
    compositeCam->setNodeMask(SceneUtil::Mask_PreCompile);
    mCompositeMapCamera = compositeCam;

    compileRoot->addChild(compositeCam);

    mCompositeMapRenderer = new CompositeMapRenderer;
    compositeCam->addChild(mCompositeMapRenderer);

    mParent->addChild(mTerrainRoot);

    mTextureManager.reset(new TextureManager(mResourceSystem->getSceneManager()));
    mChunkManager.reset(new ChunkManager(mStorage, mResourceSystem->getSceneManager(), mTextureManager.get(), mCompositeMapRenderer));
    mCellBorder.reset(new CellBorder(this,mTerrainRoot.get()));

    mResourceSystem->addResourceManager(mChunkManager.get());
    mResourceSystem->addResourceManager(mTextureManager.get());
}

World::~World()
{
    mResourceSystem->removeResourceManager(mChunkManager.get());
    mResourceSystem->removeResourceManager(mTextureManager.get());

    mParent->removeChild(mTerrainRoot);

    mCompositeMapCamera->removeChild(mCompositeMapRenderer);
    mCompositeMapCamera->getParent(0)->removeChild(mCompositeMapCamera);

    delete mStorage;
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
    mTextureManager->updateTextureFiltering();
}

void World::clearAssociatedCaches()
{
    mChunkManager->clearCache();
}

}
