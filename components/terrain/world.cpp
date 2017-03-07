#include "world.hpp"

#include <osg/Group>
#include <osgUtil/IncrementalCompileOperation>

#include <components/resource/resourcesystem.hpp>

#include "storage.hpp"
#include "texturemanager.hpp"
#include "chunkmanager.hpp"
#include "compositemaprenderer.hpp"

namespace Terrain
{

World::World(osg::Group* parent, osg::Group* compileRoot, Resource::ResourceSystem* resourceSystem, osgUtil::IncrementalCompileOperation* ico,
             Storage* storage, int nodeMask, int preCompileMask)
    : mStorage(storage)
    , mParent(parent)
    , mResourceSystem(resourceSystem)
    , mIncrementalCompileOperation(ico)
{
    mTerrainRoot = new osg::Group;
    mTerrainRoot->setNodeMask(nodeMask);
    mTerrainRoot->getOrCreateStateSet()->setRenderingHint(osg::StateSet::OPAQUE_BIN);
    mTerrainRoot->setName("Terrain Root");

    osg::ref_ptr<CompositeMapRenderer> renderer (new CompositeMapRenderer);
    renderer->setNodeMask(preCompileMask);
    compileRoot->addChild(renderer);
    mCompositeMapRenderer = renderer;

    mParent->addChild(mTerrainRoot);

    mTextureManager.reset(new TextureManager(mResourceSystem->getSceneManager()));
    mChunkManager.reset(new ChunkManager(mStorage, mResourceSystem->getSceneManager(), mTextureManager.get(), renderer));

    mResourceSystem->addResourceManager(mChunkManager.get());
    mResourceSystem->addResourceManager(mTextureManager.get());
}

World::~World()
{
    mResourceSystem->removeResourceManager(mChunkManager.get());
    mResourceSystem->removeResourceManager(mTextureManager.get());

    mParent->removeChild(mTerrainRoot);
    mCompositeMapRenderer->getParent(0)->removeChild(mCompositeMapRenderer);

    delete mStorage;
}

float World::getHeightAt(const osg::Vec3f &worldPos)
{
    return mStorage->getHeightAt(worldPos);
}

}
