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
#include "quadtreenode.hpp"

#include <OgreSceneManager.h>
#include <OgreManualObject.h>
#include <OgreSceneNode.h>
#include <OgreMaterialManager.h>
#include <OgreTextureManager.h>

#include "defaultworld.hpp"
#include "chunk.hpp"
#include "storage.hpp"
#include "buffercache.hpp"
#include "material.hpp"

using namespace Terrain;

namespace
{
    int Log2( int n )
    {
        assert(n > 0);
        int targetlevel = 0;
        while (n >>= 1) ++targetlevel;
        return targetlevel;
    }

    // Utility functions for neighbour finding algorithm
    ChildDirection reflect(ChildDirection dir, Direction dir2)
    {
        assert(dir != Root);

        const int lookupTable[4][4] =
        {
            // NW  NE  SW  SE
            {  SW, SE, NW, NE }, // N
            {  NE, NW, SE, SW }, // E
            {  SW, SE, NW, NE }, // S
            {  NE, NW, SE, SW }  // W
        };
        return (ChildDirection)lookupTable[dir2][dir];
    }

    bool adjacent(ChildDirection dir, Direction dir2)
    {
        assert(dir != Root);
        const bool lookupTable[4][4] =
        {
            // NW    NE    SW     SE
            {  true, true, false, false }, // N
            {  false, true, false, true }, // E
            {  false, false, true, true }, // S
            {  true, false, true, false }  // W
        };
        return lookupTable[dir2][dir];
    }

    // Algorithm described by Hanan Samet - 'Neighbour Finding in Quadtrees'
    // http://www.cs.umd.edu/~hjs/pubs/SametPRIP81.pdf
    QuadTreeNode* searchNeighbourRecursive (QuadTreeNode* currentNode, Direction dir)
    {
        if (!currentNode->getParent())
            return NULL; // Arrived at root node, the root node does not have neighbours

        QuadTreeNode* nextNode;
        if (adjacent(currentNode->getDirection(), dir))
            nextNode = searchNeighbourRecursive(currentNode->getParent(), dir);
        else
            nextNode = currentNode->getParent();

        if (nextNode && nextNode->hasChildren())
            return nextNode->getChild(reflect(currentNode->getDirection(), dir));
        else
            return NULL;
    }

    // Create a 2D quad
    void makeQuad(Ogre::SceneManager* sceneMgr, float left, float top, float right, float bottom, Ogre::MaterialPtr material)
    {
        Ogre::ManualObject* manual = sceneMgr->createManualObject();

        // Use identity view/projection matrices to get a 2d quad
        manual->setUseIdentityProjection(true);
        manual->setUseIdentityView(true);

        manual->begin(material->getName());

        float normLeft = left*2-1;
        float normTop = top*2-1;
        float normRight = right*2-1;
        float normBottom = bottom*2-1;

        manual->position(normLeft, normTop, 0.0);
        manual->textureCoord(0, 1);
        manual->position(normRight, normTop, 0.0);
        manual->textureCoord(1, 1);
        manual->position(normRight, normBottom, 0.0);
        manual->textureCoord(1, 0);
        manual->position(normLeft, normBottom, 0.0);
        manual->textureCoord(0, 0);

        manual->quad(0,1,2,3);

        manual->end();

        Ogre::AxisAlignedBox aabInf;
        aabInf.setInfinite();
        manual->setBoundingBox(aabInf);

        sceneMgr->getRootSceneNode()->createChildSceneNode()->attachObject(manual);
    }
}

QuadTreeNode::QuadTreeNode(DefaultWorld* terrain, ChildDirection dir, float size, const Ogre::Vector2 &center, QuadTreeNode* parent)
    : mMaterialGenerator(NULL)
    , mLoadState(LS_Unloaded)
    , mIsDummy(false)
    , mSize(size)
    , mLodLevel(Log2(static_cast<int>(mSize)))
    , mBounds(Ogre::AxisAlignedBox::BOX_NULL)
    , mWorldBounds(Ogre::AxisAlignedBox::BOX_NULL)
    , mDirection(dir)
    , mCenter(center)
    , mSceneNode(NULL)
    , mParent(parent)
    , mChunk(NULL)
    , mTerrain(terrain)
{
    mBounds.setNull();
    for (int i=0; i<4; ++i)
        mChildren[i] = NULL;
    for (int i=0; i<4; ++i)
        mNeighbours[i] = NULL;

    if (mDirection == Root)
        mSceneNode = mTerrain->getRootSceneNode();
    else
        mSceneNode = mTerrain->getSceneManager()->createSceneNode();
    Ogre::Vector2 pos (0,0);
    if (mParent)
        pos = mParent->getCenter();
    pos = mCenter - pos;
    float cellWorldSize = mTerrain->getStorage()->getCellWorldSize();

    Ogre::Vector3 sceneNodePos (pos.x*cellWorldSize, pos.y*cellWorldSize, 0);
    mTerrain->convertPosition(sceneNodePos);

    mSceneNode->setPosition(sceneNodePos);

    mMaterialGenerator = new MaterialGenerator();
    mMaterialGenerator->enableShaders(mTerrain->getShadersEnabled());
}

void QuadTreeNode::createChild(ChildDirection id, float size, const Ogre::Vector2 &center)
{
    mChildren[id] = new QuadTreeNode(mTerrain, id, size, center, this);
}

QuadTreeNode::~QuadTreeNode()
{
    for (int i=0; i<4; ++i)
        delete mChildren[i];
    delete mChunk;
    delete mMaterialGenerator;
}

QuadTreeNode* QuadTreeNode::getNeighbour(Direction dir)
{
    return mNeighbours[static_cast<int>(dir)];
}

void QuadTreeNode::initNeighbours()
{
    for (int i=0; i<4; ++i)
        mNeighbours[i] = searchNeighbourRecursive(this, (Direction)i);

    if (hasChildren())
        for (int i=0; i<4; ++i)
            mChildren[i]->initNeighbours();
}

void QuadTreeNode::initAabb()
{
    float cellWorldSize = mTerrain->getStorage()->getCellWorldSize();
    if (hasChildren())
    {
        for (int i=0; i<4; ++i)
        {
            mChildren[i]->initAabb();
            mBounds.merge(mChildren[i]->getBoundingBox());
        }
        float minH, maxH;
        switch (mTerrain->getAlign())
        {
            case Terrain::Align_XY:
                minH = mBounds.getMinimum().z;
                maxH = mBounds.getMaximum().z;
                break;
            case Terrain::Align_XZ:
                minH = mBounds.getMinimum().y;
                maxH = mBounds.getMaximum().y;
                break;
            case Terrain::Align_YZ:
                minH = mBounds.getMinimum().x;
                maxH = mBounds.getMaximum().x;
                break;
        }
        Ogre::Vector3 min(-mSize/2*cellWorldSize, -mSize/2*cellWorldSize, minH);
        Ogre::Vector3 max(Ogre::Vector3(mSize/2*cellWorldSize, mSize/2*cellWorldSize, maxH));
        mBounds = Ogre::AxisAlignedBox (min, max);
        mTerrain->convertBounds(mBounds);
    }
    Ogre::Vector3 offset(mCenter.x*cellWorldSize, mCenter.y*cellWorldSize, 0);
    mTerrain->convertPosition(offset);
    mWorldBounds = Ogre::AxisAlignedBox(mBounds.getMinimum() + offset,
                                        mBounds.getMaximum() + offset);
}

void QuadTreeNode::setBoundingBox(const Ogre::AxisAlignedBox &box)
{
    mBounds = box;
}

const Ogre::AxisAlignedBox& QuadTreeNode::getBoundingBox()
{
    return mBounds;
}

const Ogre::AxisAlignedBox& QuadTreeNode::getWorldBoundingBox()
{
    return mWorldBounds;
}

bool QuadTreeNode::update(const Ogre::Vector3 &cameraPos)
{
    if (isDummy())
        return true;

    if (mBounds.isNull())
        return true;

    float dist = mWorldBounds.distance(cameraPos);

    // Make sure our scene node is attached
    if (!mSceneNode->isInSceneGraph())
    {
        mParent->getSceneNode()->addChild(mSceneNode);
    }

    // Simple LOD selection
    /// \todo use error metrics?
    size_t wantedLod = 0;
    float cellWorldSize = mTerrain->getStorage()->getCellWorldSize();

    if (dist > cellWorldSize*64)
        wantedLod = 6;
    else if (dist > cellWorldSize*32)
        wantedLod = 5;
    else if (dist > cellWorldSize*12)
        wantedLod = 4;
    else if (dist > cellWorldSize*5)
        wantedLod = 3;
    else if (dist > cellWorldSize*2)
        wantedLod = 2;
    else if (dist > cellWorldSize * 1.42) // < sqrt2 so the 3x3 grid around player is always highest lod
        wantedLod = 1;

    bool wantToDisplay = mSize <= mTerrain->getMaxBatchSize() && mLodLevel <= wantedLod;

    if (wantToDisplay)
    {
        // Wanted LOD is small enough to render this node in one chunk
        if (mLoadState == LS_Unloaded)
        {
            mLoadState = LS_Loading;
            mTerrain->queueLoad(this);
            return false;
        }

        if (mLoadState == LS_Loaded)
        {
            // Additional (index buffer) LOD is currently disabled.
            // This is due to a problem with the LOD selection when a node splits.
            // After splitting, the distance is measured from the children's bounding boxes, which are possibly
            // further away than the original node's bounding box, possibly causing a child to switch to a *lower* LOD
            // than the original node.
            // In short, we'd sometimes get a switch to a lesser detail when actually moving closer.
            // This wouldn't be so bad, but unfortunately it also breaks LOD edge connections if a neighbour
            // node hasn't split yet, and has a higher LOD than our node's child:
            //  ----- ----- ------------
            // | LOD | LOD |            |
            // |  1  |  1  |            |
            // |-----|-----|   LOD 0    |
            // | LOD | LOD |            |
            // |  0  |  0  |            |
            //  ----- ----- ------------
            // To prevent this, nodes of the same size need to always select the same LOD, which is basically what we're
            // doing here.
            // But this "solution" does increase triangle overhead, so eventually we need to find a more clever way.
            //mChunk->setAdditionalLod(wantedLod - mLodLevel);

            if (!mChunk->getVisible() && hasChildren())
            {
                for (int i=0; i<4; ++i)
                    mChildren[i]->unload(true);
            }
            mChunk->setVisible(true);

            return true;
        }
        return false; // LS_Loading
    }

    // We do not want to display this node - delegate to children if they are already loaded
    if (!wantToDisplay && hasChildren())
    {
        if (mChunk)
        {
            // Are children already loaded?
            bool childrenLoaded = true;
            for (int i=0; i<4; ++i)
                if (!mChildren[i]->update(cameraPos))
                    childrenLoaded = false;

            if (!childrenLoaded)
            {
                mChunk->setVisible(true);
                // Make sure child scene nodes are detached until all children are loaded
                mSceneNode->removeAllChildren();
            }
            else
            {
                // Delegation went well, we can unload now
                unload();

                for (int i=0; i<4; ++i)
                {
                    if (!mChildren[i]->getSceneNode()->isInSceneGraph())
                        mSceneNode->addChild(mChildren[i]->getSceneNode());
                }
            }
            return true;
        }
        else
        {
            bool success = true;
            for (int i=0; i<4; ++i)
                success = mChildren[i]->update(cameraPos) & success;
            return success;
        }
    }
    return false;
}

void QuadTreeNode::load(const LoadResponseData &data)
{
    assert (!mChunk);

    mChunk = new Chunk(mTerrain->getBufferCache().getUVBuffer(), mBounds, data.mPositions, data.mNormals, data.mColours);
    mChunk->setVisibilityFlags(mTerrain->getVisibilityFlags());
    mChunk->setCastShadows(true);
    mSceneNode->attachObject(mChunk);

    mMaterialGenerator->enableShadows(mTerrain->getShadowsEnabled());
    mMaterialGenerator->enableSplitShadows(mTerrain->getSplitShadowsEnabled());

    if (mTerrain->areLayersLoaded())
    {
        if (mSize == 1)
        {
            mChunk->setMaterial(mMaterialGenerator->generate());
        }
        else
        {
            ensureCompositeMap();
            mMaterialGenerator->setCompositeMap(mCompositeMap->getName());
            mChunk->setMaterial(mMaterialGenerator->generateForCompositeMap());
        }
    }
    // else: will be loaded in loadMaterials() after background thread has finished loading layers
    mChunk->setVisible(false);

    mLoadState = LS_Loaded;
}

void QuadTreeNode::unload(bool recursive)
{
    if (mChunk)
    {
        mSceneNode->detachObject(mChunk);

        delete mChunk;
        mChunk = NULL;

        if (!mCompositeMap.isNull())
        {
            Ogre::TextureManager::getSingleton().remove(mCompositeMap->getName());
            mCompositeMap.setNull();
        }

        // Do *not* set this when we are still loading!
        mLoadState = LS_Unloaded;
    }

    if (recursive && hasChildren())
    {
        for (int i=0; i<4; ++i)
            mChildren[i]->unload(true);
    }
}

void QuadTreeNode::updateIndexBuffers()
{
    if (hasChunk())
    {
        // Fetch a suitable index buffer (which may be shared)
        size_t ourLod = getActualLodLevel();

        unsigned int flags = 0;

        for (int i=0; i<4; ++i)
        {
            QuadTreeNode* neighbour = getNeighbour((Direction)i);

            // If the neighbour isn't currently rendering itself,
            // go up until we find one. NOTE: We don't need to go down,
            // because in that case neighbour's detail would be higher than
            // our detail and the neighbour would handle stitching by itself.
            while (neighbour && !neighbour->hasChunk())
                neighbour = neighbour->getParent();
            size_t lod = 0;
            if (neighbour)
                lod = neighbour->getActualLodLevel();
            if (lod <= ourLod) // We only need to worry about neighbours less detailed than we are -
                lod = 0;         // neighbours with more detail will do the stitching themselves
            // Use 4 bits for each LOD delta
            if (lod > 0)
            {
                assert (lod - ourLod < (1 << 4));
                flags |= static_cast<unsigned int>(lod - ourLod) << (4*i);
            }
        }
        flags |= 0 /*((int)mAdditionalLod)*/ << (4*4);

        mChunk->setIndexBuffer(mTerrain->getBufferCache().getIndexBuffer(flags));
    }
    else if (hasChildren())
    {
        for (int i=0; i<4; ++i)
            mChildren[i]->updateIndexBuffers();
    }
}

bool QuadTreeNode::hasChunk()
{
    return mSceneNode->isInSceneGraph() && mChunk && mChunk->getVisible();
}

size_t QuadTreeNode::getActualLodLevel()
{
    assert(hasChunk() && "Can't get actual LOD level if this node has no render chunk");
    return mLodLevel /* + mChunk->getAdditionalLod() */;
}

void QuadTreeNode::loadLayers(const LayerCollection& collection)
{
    assert (!mMaterialGenerator->hasLayers());

    std::vector<Ogre::TexturePtr> blendTextures;
    for (std::vector<Ogre::PixelBox>::const_iterator it = collection.mBlendmaps.begin(); it != collection.mBlendmaps.end(); ++it)
    {
        // TODO: clean up blend textures on destruction
        static int count=0;
        Ogre::TexturePtr map = Ogre::TextureManager::getSingleton().createManual("terrain/blend/"
            + Ogre::StringConverter::toString(count++), Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
            Ogre::TEX_TYPE_2D, it->getWidth(), it->getHeight(), 0, it->format);

        Ogre::DataStreamPtr stream(new Ogre::MemoryDataStream(it->data, it->getWidth()*it->getHeight()*Ogre::PixelUtil::getNumElemBytes(it->format), true));
        map->loadRawData(stream, it->getWidth(), it->getHeight(), it->format);
        blendTextures.push_back(map);
    }

    mMaterialGenerator->setLayerList(collection.mLayers);
    mMaterialGenerator->setBlendmapList(blendTextures);
}

void QuadTreeNode::loadMaterials()
{
    if (isDummy())
        return;

    // Load children first since we depend on them when creating a composite map
    if (hasChildren())
    {
        for (int i=0; i<4; ++i)
            mChildren[i]->loadMaterials();
    }

    if (mChunk)
    {
        if (mSize == 1)
        {
            mChunk->setMaterial(mMaterialGenerator->generate());
        }
        else
        {
            ensureCompositeMap();
            mMaterialGenerator->setCompositeMap(mCompositeMap->getName());
            mChunk->setMaterial(mMaterialGenerator->generateForCompositeMap());
        }
    }
}

void QuadTreeNode::prepareForCompositeMap(Ogre::TRect<float> area)
{
    Ogre::SceneManager* sceneMgr = mTerrain->getCompositeMapSceneManager();

    if (mIsDummy)
    {
        // TODO - store this default material somewhere instead of creating one for each empty cell
        MaterialGenerator matGen;
        matGen.enableShaders(mTerrain->getShadersEnabled());
        std::vector<LayerInfo> layer;
        layer.push_back(mTerrain->getStorage()->getDefaultLayer());
        matGen.setLayerList(layer);
        makeQuad(sceneMgr, area.left, area.top, area.right, area.bottom, matGen.generateForCompositeMapRTT());
        return;
    }
    if (mSize > 1)
    {
        assert(hasChildren());

        // 0,0 -------- 1,0
        //  |     |      |
        //  |-----|------|
        //  |     |      |
        // 0,1 -------- 1,1

        float halfW = area.width()/2.f;
        float halfH = area.height()/2.f;
        mChildren[NW]->prepareForCompositeMap(Ogre::TRect<float>(area.left, area.top, area.right-halfW, area.bottom-halfH));
        mChildren[NE]->prepareForCompositeMap(Ogre::TRect<float>(area.left+halfW, area.top, area.right, area.bottom-halfH));
        mChildren[SW]->prepareForCompositeMap(Ogre::TRect<float>(area.left, area.top+halfH, area.right-halfW, area.bottom));
        mChildren[SE]->prepareForCompositeMap(Ogre::TRect<float>(area.left+halfW, area.top+halfH, area.right, area.bottom));
    }
    else
    {
        // TODO: when to destroy?
        Ogre::MaterialPtr material = mMaterialGenerator->generateForCompositeMapRTT();
        makeQuad(sceneMgr, area.left, area.top, area.right, area.bottom, material);
    }
}

void QuadTreeNode::ensureCompositeMap()
{
    if (!mCompositeMap.isNull())
        return;

    static int i=0;
    std::stringstream name;
    name << "terrain/comp" << i++;

    const int size = 128;
    mCompositeMap = Ogre::TextureManager::getSingleton().createManual(
                name.str(), Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
        Ogre::TEX_TYPE_2D, size, size, Ogre::MIP_DEFAULT, Ogre::PF_A8B8G8R8);

    // Create quads for each cell
    prepareForCompositeMap(Ogre::TRect<float>(0,0,1,1));

    mTerrain->renderCompositeMap(mCompositeMap);

    mTerrain->clearCompositeMapSceneManager();

}

void QuadTreeNode::applyMaterials()
{
    if (mChunk)
    {
        mMaterialGenerator->enableShadows(mTerrain->getShadowsEnabled());
        mMaterialGenerator->enableSplitShadows(mTerrain->getSplitShadowsEnabled());
        if (mSize <= 1)
            mChunk->setMaterial(mMaterialGenerator->generate());
        else
            mChunk->setMaterial(mMaterialGenerator->generateForCompositeMap());
    }
    if (hasChildren())
        for (int i=0; i<4; ++i)
            mChildren[i]->applyMaterials();
}
