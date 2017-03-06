#include "world.hpp"

#include <osg/Group>
#include <osgUtil/IncrementalCompileOperation>

#include <components/resource/resourcesystem.hpp>

#include "storage.hpp"
#include "texturemanager.hpp"

namespace Terrain
{

World::World(osg::Group* parent, Resource::ResourceSystem* resourceSystem, osgUtil::IncrementalCompileOperation* ico,
             Storage* storage, int nodeMask)
    : mStorage(storage)
    , mParent(parent)
    , mResourceSystem(resourceSystem)
    , mIncrementalCompileOperation(ico)
{
    mTerrainRoot = new osg::Group;
    mTerrainRoot->setNodeMask(nodeMask);
    mTerrainRoot->getOrCreateStateSet()->setRenderingHint(osg::StateSet::OPAQUE_BIN);
    mTerrainRoot->setName("Terrain Root");

    mParent->addChild(mTerrainRoot);

    mTextureManager.reset(new TextureManager(mResourceSystem->getSceneManager()));

    mResourceSystem->addResourceManager(mTextureManager.get());
}

World::~World()
{
    mResourceSystem->removeResourceManager(mTextureManager.get());

    mParent->removeChild(mTerrainRoot);

    delete mStorage;
}

float World::getHeightAt(const osg::Vec3f &worldPos)
{
    return mStorage->getHeightAt(worldPos);
}

}
