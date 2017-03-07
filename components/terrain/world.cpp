#include "world.hpp"

#include <osg/Group>
#include <osg/Material>

#include <components/resource/resourcesystem.hpp>

#include "storage.hpp"
#include "texturemanager.hpp"
#include "chunkmanager.hpp"
#include "compositemaprenderer.hpp"

namespace Terrain
{

World::World(osg::Group* parent, osg::Group* compileRoot, Resource::ResourceSystem* resourceSystem, Storage* storage, int nodeMask, int preCompileMask)
    : mStorage(storage)
    , mParent(parent)
    , mResourceSystem(resourceSystem)
{
    mTerrainRoot = new osg::Group;
    mTerrainRoot->setNodeMask(nodeMask);
    mTerrainRoot->getOrCreateStateSet()->setRenderingHint(osg::StateSet::OPAQUE_BIN);
    osg::ref_ptr<osg::Material> material (new osg::Material);
    material->setColorMode(osg::Material::AMBIENT_AND_DIFFUSE);
    mTerrainRoot->getOrCreateStateSet()->setAttributeAndModes(material, osg::StateAttribute::ON);

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

void World::updateTextureFiltering()
{
    mTextureManager->updateTextureFiltering();
}

}
