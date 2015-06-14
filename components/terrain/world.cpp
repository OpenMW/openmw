#include "world.hpp"

#include <osg/Group>
#include <osgUtil/IncrementalCompileOperation>

#include "storage.hpp"

namespace Terrain
{

World::World(osg::Group* parent, Resource::ResourceSystem* resourceSystem, osgUtil::IncrementalCompileOperation* ico,
             Storage* storage, int nodeMask)
    : mStorage(storage)
    , mCache(storage->getCellVertices())
    , mParent(parent)
    , mResourceSystem(resourceSystem)
    , mIncrementalCompileOperation(ico)
{
    mTerrainRoot = new osg::Group;
    mTerrainRoot->setNodeMask(nodeMask);
    mTerrainRoot->getOrCreateStateSet()->setRenderingHint(osg::StateSet::OPAQUE_BIN);

    mParent->addChild(mTerrainRoot);
}

World::~World()
{
    mParent->removeChild(mTerrainRoot);

    delete mStorage;
}

float World::getHeightAt(const osg::Vec3f &worldPos)
{
    return mStorage->getHeightAt(worldPos);
}

}
