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
#include "terraingrid.hpp"

#include <OgreSceneManager.h>
#include <OgreSceneNode.h>
#include <OgreAxisAlignedBox.h>

#include "chunk.hpp"

namespace Terrain
{

TerrainGrid::TerrainGrid(Ogre::SceneManager *sceneMgr, Terrain::Storage *storage, int visibilityFlags, bool shaders, Terrain::Alignment align)
    : Terrain::World(sceneMgr, storage, visibilityFlags, shaders, align)
    , mVisible(true)
{
    mRootNode = mSceneMgr->getRootSceneNode()->createChildSceneNode();
}

TerrainGrid::~TerrainGrid()
{
    while (!mGrid.empty())
    {
        unloadCell(mGrid.begin()->first.first, mGrid.begin()->first.second);
    }

    mSceneMgr->destroySceneNode(mRootNode);
}

void TerrainGrid::update(const Ogre::Vector3 &cameraPos)
{
}

void TerrainGrid::loadCell(int x, int y)
{
    if (mGrid.find(std::make_pair(x, y)) != mGrid.end())
        return; // already loaded

    Ogre::Vector2 center(x+0.5f, y+0.5f);
    float minH, maxH;
    if (!mStorage->getMinMaxHeights(1, center, minH, maxH))
        return; // no terrain defined

    Ogre::Vector3 min (-0.5f*mStorage->getCellWorldSize(),
                       -0.5f*mStorage->getCellWorldSize(),
                       minH);
    Ogre::Vector3 max (0.5f*mStorage->getCellWorldSize(),
                       0.5f*mStorage->getCellWorldSize(),
                       maxH);

    Ogre::AxisAlignedBox bounds(min, max);

    GridElement element;

    Ogre::Vector2 worldCenter = center*mStorage->getCellWorldSize();
    element.mSceneNode = mRootNode->createChildSceneNode(Ogre::Vector3(worldCenter.x, worldCenter.y, 0));

    std::vector<float> positions;
    std::vector<float> normals;
    std::vector<Ogre::uint8> colours;
    mStorage->fillVertexBuffers(0, 1, center, mAlign, positions, normals, colours);

    element.mChunk = new Terrain::Chunk(mCache.getUVBuffer(), bounds, positions, normals, colours);
    element.mChunk->setIndexBuffer(mCache.getIndexBuffer(0));
    element.mChunk->setVisibilityFlags(mVisibilityFlags);
    element.mChunk->setCastShadows(true);

    std::vector<Ogre::PixelBox> blendmaps;
    std::vector<Terrain::LayerInfo> layerList;
    mStorage->getBlendmaps(1, center, mShaders, blendmaps, layerList);

    element.mMaterialGenerator.setLayerList(layerList);

    // upload blendmaps to GPU
    std::vector<Ogre::TexturePtr> blendTextures;
    for (std::vector<Ogre::PixelBox>::const_iterator it = blendmaps.begin(); it != blendmaps.end(); ++it)
    {
        static int count=0;
        Ogre::TexturePtr map = Ogre::TextureManager::getSingleton().createManual("terrain/blend/"
            + Ogre::StringConverter::toString(count++), Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
            Ogre::TEX_TYPE_2D, it->getWidth(), it->getHeight(), 0, it->format);

        Ogre::DataStreamPtr stream(new Ogre::MemoryDataStream(it->data, it->getWidth()*it->getHeight()*Ogre::PixelUtil::getNumElemBytes(it->format), true));
        map->loadRawData(stream, it->getWidth(), it->getHeight(), it->format);
        blendTextures.push_back(map);
    }

    element.mMaterialGenerator.setBlendmapList(blendTextures);

    element.mSceneNode->attachObject(element.mChunk);
    updateMaterial(element);

    mGrid[std::make_pair(x,y)] = element;
}

void TerrainGrid::updateMaterial(GridElement &element)
{
    element.mMaterialGenerator.enableShadows(getShadowsEnabled());
    element.mMaterialGenerator.enableSplitShadows(getSplitShadowsEnabled());
    element.mChunk->setMaterial(element.mMaterialGenerator.generate());
}

void TerrainGrid::unloadCell(int x, int y)
{
    Grid::iterator it = mGrid.find(std::make_pair(x,y));
    if (it == mGrid.end())
        return;

    GridElement& element = it->second;
    delete element.mChunk;
    element.mChunk = NULL;

    const std::vector<Ogre::TexturePtr>& blendmaps = element.mMaterialGenerator.getBlendmapList();
    for (std::vector<Ogre::TexturePtr>::const_iterator it = blendmaps.begin(); it != blendmaps.end(); ++it)
        Ogre::TextureManager::getSingleton().remove((*it)->getName());

    mSceneMgr->destroySceneNode(element.mSceneNode);
    element.mSceneNode = NULL;

    mGrid.erase(it);
}

void TerrainGrid::applyMaterials(bool shadows, bool splitShadows)
{
    mShadows = shadows;
    mSplitShadows = splitShadows;
    for (Grid::iterator it = mGrid.begin(); it != mGrid.end(); ++it)
    {
        updateMaterial(it->second);
    }
}

bool TerrainGrid::getVisible()
{
    return mVisible;
}

void TerrainGrid::setVisible(bool visible)
{
    mVisible = visible;
    mRootNode->setVisible(visible);
}

Ogre::AxisAlignedBox TerrainGrid::getWorldBoundingBox (const Ogre::Vector2& center)
{
    int cellX = static_cast<int>(std::floor(center.x));
    int cellY = static_cast<int>(std::floor(center.y));

    Grid::iterator it = mGrid.find(std::make_pair(cellX, cellY));
    if (it == mGrid.end())
        return Ogre::AxisAlignedBox::BOX_NULL;

    Terrain::Chunk* chunk = it->second.mChunk;
    Ogre::SceneNode* node = it->second.mSceneNode;
    Ogre::AxisAlignedBox box = chunk->getBoundingBox();
    box = Ogre::AxisAlignedBox(box.getMinimum() + node->getPosition(), box.getMaximum() + node->getPosition());
    return box;
}

void TerrainGrid::syncLoad()
{

}

}
