#include "quadtreenode.hpp"

#include <OgreSceneManager.h>
#include <OgreManualObject.h>

#include "terrain.hpp"
#include "chunk.hpp"
#include "storage.hpp"

#include "material.hpp"

using namespace Terrain;

namespace
{

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
    Terrain::QuadTreeNode* searchNeighbourRecursive (Terrain::QuadTreeNode* currentNode, Terrain::Direction dir)
    {
        if (!currentNode->getParent())
            return NULL; // Arrived at root node, the root node does not have neighbours

        Terrain::QuadTreeNode* nextNode;
        if (adjacent(currentNode->getDirection(), dir))
            nextNode = searchNeighbourRecursive(currentNode->getParent(), dir);
        else
            nextNode = currentNode->getParent();

        if (nextNode && nextNode->hasChildren())
            return nextNode->getChild(reflect(currentNode->getDirection(), dir));
        else
            return NULL;
    }


    // Ogre::AxisAlignedBox::distance is broken in 1.8.
    Ogre::Real distance(const Ogre::AxisAlignedBox& box, const Ogre::Vector3& v)
    {

      if (box.contains(v))
        return 0;
      else
      {
          Ogre::Vector3 maxDist(0,0,0);
        const Ogre::Vector3& minimum = box.getMinimum();
        const Ogre::Vector3& maximum = box.getMaximum();

        if (v.x < minimum.x)
          maxDist.x = minimum.x - v.x;
        else if (v.x > maximum.x)
          maxDist.x = v.x - maximum.x;

        if (v.y < minimum.y)
          maxDist.y = minimum.y - v.y;
        else if (v.y > maximum.y)
          maxDist.y = v.y - maximum.y;

        if (v.z < minimum.z)
          maxDist.z = minimum.z - v.z;
        else if (v.z > maximum.z)
          maxDist.z = v.z - maximum.z;

        return maxDist.length();
      }
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

QuadTreeNode::QuadTreeNode(Terrain* terrain, ChildDirection dir, float size, const Ogre::Vector2 &center, QuadTreeNode* parent)
    : mSize(size)
    , mCenter(center)
    , mParent(parent)
    , mDirection(dir)
    , mIsDummy(false)
    , mSceneNode(NULL)
    , mTerrain(terrain)
    , mChunk(NULL)
    , mMaterialGenerator(NULL)
{
    mBounds.setNull();
    for (int i=0; i<4; ++i)
        mChildren[i] = NULL;

    mSceneNode = mTerrain->getSceneManager()->getRootSceneNode()->createChildSceneNode(
                Ogre::Vector3(mCenter.x*8192, mCenter.y*8192, 0));

    mLodLevel = log2(mSize);

    mMaterialGenerator = new MaterialGenerator(true);
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

QuadTreeNode* QuadTreeNode::searchNeighbour(Direction dir)
{
    return searchNeighbourRecursive(this, dir);
}

const Ogre::AxisAlignedBox& QuadTreeNode::getBoundingBox()
{
    if (mIsDummy)
        return Ogre::AxisAlignedBox::BOX_NULL;
    if (mBounds.isNull())
    {
        if (hasChildren())
        {
            // X and Y are obvious, just need Z
            float min = std::numeric_limits<float>().max();
            float max = -std::numeric_limits<float>().max();
            for (int i=0; i<4; ++i)
            {
                QuadTreeNode* child = getChild((ChildDirection)i);
                float v = child->getBoundingBox().getMaximum().z;
                if (v > max)
                    max = v;
                v = child->getBoundingBox().getMinimum().z;
                if (v < min)
                    min = v;
            }
            mBounds = Ogre::AxisAlignedBox (Ogre::Vector3(-mSize/2*8192, -mSize/2*8192, min),
                                            Ogre::Vector3(mSize/2*8192, mSize/2*8192, max));
        }
        else
            throw std::runtime_error("Leaf node should have bounds set!");
    }
    return mBounds;
}

void QuadTreeNode::update(const Ogre::Vector3 &cameraPos)
{
    const Ogre::AxisAlignedBox& bounds = getBoundingBox();
    if (bounds.isNull())
        return;

    Ogre::AxisAlignedBox worldBounds (bounds.getMinimum() + Ogre::Vector3(mCenter.x*8192, mCenter.y*8192, 0),
                                      bounds.getMaximum() + Ogre::Vector3(mCenter.x*8192, mCenter.y*8192, 0));

    float dist = distance(worldBounds, cameraPos);
    /// \todo implement error metrics or some other means of not using arbitrary values
    size_t wantedLod = 0;
    if (dist > 8192*1)
        wantedLod = 1;
    if (dist > 8192*2)
        wantedLod = 2;
    if (dist > 8192*5)
        wantedLod = 3;
    if (dist > 8192*12)
        wantedLod = 4;
    if (dist > 8192*32)
        wantedLod = 5;
    if (dist > 8192*64)
        wantedLod = 6;

    if (mSize <= mTerrain->getMaxBatchSize() && mLodLevel <= wantedLod)
    {
        // Wanted LOD is small enough to render this node in one chunk
        if (!mChunk)
        {
            mChunk = new Chunk(this, mLodLevel);
            mChunk->setVisibilityFlags(mTerrain->getVisiblityFlags());
            mChunk->setCastShadows(true);
            mSceneNode->attachObject(mChunk);
            if (mSize == 1)
            {
                ensureLayerInfo();
                mChunk->setMaterial(mMaterialGenerator->generate(mChunk->getMaterial()));
            }
            else
            {
                ensureCompositeMap();
                mMaterialGenerator->setCompositeMap(mCompositeMap->getName());
                mChunk->setMaterial(mMaterialGenerator->generateForCompositeMap(mChunk->getMaterial()));
            }
        }


        mChunk->setAdditionalLod(wantedLod - mLodLevel);
        mChunk->setVisible(true);

        if (hasChildren())
        {
            for (int i=0; i<4; ++i)
                mChildren[i]->removeChunks();
        }
    }
    else
    {
        // Wanted LOD is too detailed to be rendered in one chunk,
        // so split it up by delegating to child nodes
        if (mChunk)
            mChunk->setVisible(false);
        assert(hasChildren() && "Leaf node's LOD needs to be 0");
        for (int i=0; i<4; ++i)
            mChildren[i]->update(cameraPos);
    }
}

void QuadTreeNode::removeChunks()
{
    if (mChunk)
        mChunk->setVisible(false);
    if (hasChildren())
    {
        for (int i=0; i<4; ++i)
            mChildren[i]->removeChunks();
    }
}

void QuadTreeNode::updateIndexBuffers()
{
    if (hasChunk())
        mChunk->updateIndexBuffer();
    else if (hasChildren())
    {
        for (int i=0; i<4; ++i)
            mChildren[i]->updateIndexBuffers();
    }
}

bool QuadTreeNode::hasChunk()
{
    return mChunk && mChunk->getVisible();
}

size_t QuadTreeNode::getActualLodLevel()
{
    assert(hasChunk() && "Can't get actual LOD level if this node has no render chunk");
    return mLodLevel + mChunk->getAdditionalLod();
}

void QuadTreeNode::ensureLayerInfo()
{
    if (mMaterialGenerator->hasLayers())
        return;

    std::vector<Ogre::TexturePtr> blendmaps;
    std::vector<std::string> layerList;
    mTerrain->getStorage()->getBlendmaps(mSize, mCenter, true, blendmaps, layerList);

    mMaterialGenerator->setLayerList(layerList);
    mMaterialGenerator->setBlendmapList(blendmaps);
}

void QuadTreeNode::prepareForCompositeMap(Ogre::TRect<float> area)
{
    Ogre::SceneManager* sceneMgr = mTerrain->getCompositeMapSceneManager();

    if (mIsDummy)
    {
        MaterialGenerator matGen(true);
        std::vector<std::string> layer;
        layer.push_back("_land_default.dds");
        matGen.setLayerList(layer);
        makeQuad(sceneMgr, area.left, area.top, area.right, area.bottom, matGen.generate(Ogre::MaterialPtr()));
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
        ensureLayerInfo();

        Ogre::MaterialPtr material = mMaterialGenerator->generateForCompositeMapRTT(Ogre::MaterialPtr());
        makeQuad(sceneMgr, area.left, area.top, area.right, area.bottom, material);
    }
}


bool QuadTreeNode::hasCompositeMap()
{
    return !mCompositeMap.isNull();
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
