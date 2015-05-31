/*
 * Copyright (c) 2015 scrawl <scrawl@baseoftrash.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include "world.hpp"

#include <OgreAxisAlignedBox.h>

#include "storage.hpp"

namespace Terrain
{

World::World(Ogre::SceneManager* sceneMgr,
                 Storage* storage, int visibilityFlags, bool shaders, Alignment align)
    : mShaders(shaders)
    , mShadows(false)
    , mSplitShadows(false)
    , mAlign(align)
    , mStorage(storage)
    , mVisibilityFlags(visibilityFlags)
    , mSceneMgr(sceneMgr)
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
