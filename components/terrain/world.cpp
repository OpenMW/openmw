#include "world.hpp"

#include <OgreAxisAlignedBox.h>

#include "storage.hpp"

namespace Terrain
{

World::World(Ogre::SceneManager* sceneMgr,
                 Storage* storage, int visibilityFlags, bool shaders, Alignment align)
    : mStorage(storage)
    , mSceneMgr(sceneMgr)
    , mVisibilityFlags(visibilityFlags)
    , mShaders(shaders)
    , mAlign(align)
    , mCache(storage->getCellVertices())
{
}

World::~World()
{
    delete mStorage;
}

float World::getHeightAt(const Ogre::Vector3 &worldPos)
{
    return mStorage->getHeightAt(worldPos);
}

void World::convertPosition(float &x, float &y, float &z)
{
    Terrain::convertPosition(mAlign, x, y, z);
}

void World::convertPosition(Ogre::Vector3 &pos)
{
    convertPosition(pos.x, pos.y, pos.z);
}

void World::convertBounds(Ogre::AxisAlignedBox& bounds)
{
    switch (mAlign)
    {
    case Align_XY:
        return;
    case Align_XZ:
        convertPosition(bounds.getMinimum());
        convertPosition(bounds.getMaximum());
        // Because we changed sign of Z
        std::swap(bounds.getMinimum().z, bounds.getMaximum().z);
        return;
    case Align_YZ:
        convertPosition(bounds.getMinimum());
        convertPosition(bounds.getMaximum());
        return;
    }
}

}
