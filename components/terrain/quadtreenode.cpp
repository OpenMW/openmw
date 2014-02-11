#include "quadtreenode.hpp"

#include <OgreSceneManager.h>
#include <OgreManualObject.h>

#include "world.hpp"
#include "chunk.hpp"
#include "storage.hpp"

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

QuadTreeNode::QuadTreeNode(World* terrain, ChildDirection dir, float size, const Ogre::Vector2 &center, QuadTreeNode* parent)
    : mMaterialGenerator(NULL)
    , mIsActive(false)
    , mIsDummy(false)
    , mSize(size)
    , mLodLevel(Log2(mSize))
    , mBounds(Ogre::AxisAlignedBox::BOX_NULL)
    , mWorldBounds(Ogre::AxisAlignedBox::BOX_NULL)
    , mDirection(dir)
    , mCenter(center)
    , mSceneNode(NULL)
    , mParent(parent)
    , mTerrain(terrain)
    , mChunk(NULL)
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
    mSceneNode->setPosition(Ogre::Vector3(pos.x*8192, pos.y*8192, 0));

    mMaterialGenerator = new MaterialGenerator(mTerrain->getShadersEnabled());
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
    if (hasChildren())
    {
        for (int i=0; i<4; ++i)
        {
            mChildren[i]->initAabb();
            mBounds.merge(mChildren[i]->getBoundingBox());
        }
        mBounds = Ogre::AxisAlignedBox (Ogre::Vector3(-mSize/2*8192, -mSize/2*8192, mBounds.getMinimum().z),
                                        Ogre::Vector3(mSize/2*8192, mSize/2*8192, mBounds.getMaximum().z));
    }
    mWorldBounds = Ogre::AxisAlignedBox(mBounds.getMinimum() + Ogre::Vector3(mCenter.x*8192, mCenter.y*8192, 0),
                                        mBounds.getMaximum() + Ogre::Vector3(mCenter.x*8192, mCenter.y*8192, 0));
}

void QuadTreeNode::setBoundingBox(const Ogre::AxisAlignedBox &box)
{
    mBounds = box;
}

const Ogre::AxisAlignedBox& QuadTreeNode::getBoundingBox()
{
    return mBounds;
}

void QuadTreeNode::update(const Ogre::Vector3 &cameraPos, Loading::Listener* loadingListener)
{
    const Ogre::AxisAlignedBox& bounds = getBoundingBox();
    if (bounds.isNull())
        return;

    float dist = distance(mWorldBounds, cameraPos);

    bool distantLand = mTerrain->getDistantLandEnabled();

    // Make sure our scene node is attached
    if (!mSceneNode->isInSceneGraph())
    {
        mParent->getSceneNode()->addChild(mSceneNode);
    }

    /// \todo implement error metrics or some other means of not using arbitrary values
    ///  (general quality needs to be user configurable as well)
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

    bool hadChunk = hasChunk();

    if (loadingListener)
        loadingListener->indicateProgress();

    if (!distantLand && dist > 8192*2)
    {
        if (mIsActive)
        {
            destroyChunks(true);
            mIsActive = false;
        }
        return;
    }

    mIsActive = true;

    if (mSize <= mTerrain->getMaxBatchSize() && mLodLevel <= wantedLod)
    {
        // Wanted LOD is small enough to render this node in one chunk
        if (!mChunk)
        {
            mChunk = new Chunk(this, mLodLevel);
            mChunk->setVisibilityFlags(mTerrain->getVisiblityFlags());
            mChunk->setCastShadows(true);
            mSceneNode->attachObject(mChunk);

            mMaterialGenerator->enableShadows(mTerrain->getShadowsEnabled());
            mMaterialGenerator->enableSplitShadows(mTerrain->getSplitShadowsEnabled());

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

        mChunk->setVisible(true);

        if (!hadChunk && hasChildren())
        {
            // Make sure child scene nodes are detached
            mSceneNode->removeAllChildren();

            // If distant land is enabled, keep the chunks around in case we need them again,
            // otherwise, prefer low memory usage
            if (!distantLand)
                for (int i=0; i<4; ++i)
                    mChildren[i]->destroyChunks(true);
        }
    }
    else
    {
        // Wanted LOD is too detailed to be rendered in one chunk,
        // so split it up by delegating to child nodes
        if (hadChunk)
        {
            // If distant land is enabled, keep the chunks around in case we need them again,
            // otherwise, prefer low memory usage
            if (!distantLand)
                destroyChunks(false);
            else if (mChunk)
                mChunk->setVisible(false);
        }
        assert(hasChildren() && "Leaf node's LOD needs to be 0");
        for (int i=0; i<4; ++i)
            mChildren[i]->update(cameraPos, loadingListener);
    }
}

void QuadTreeNode::destroyChunks(bool children)
{
    if (mChunk)
    {
        Ogre::MaterialManager::getSingleton().remove(mChunk->getMaterial()->getName());
        mSceneNode->detachObject(mChunk);

        delete mChunk;
        mChunk = NULL;
        // destroy blendmaps
        if (mMaterialGenerator)
        {
            const std::vector<Ogre::TexturePtr>& list = mMaterialGenerator->getBlendmapList();
            for (std::vector<Ogre::TexturePtr>::const_iterator it = list.begin(); it != list.end(); ++it)
                Ogre::TextureManager::getSingleton().remove((*it)->getName());
            mMaterialGenerator->setBlendmapList(std::vector<Ogre::TexturePtr>());
            mMaterialGenerator->setLayerList(std::vector<LayerInfo>());
            mMaterialGenerator->setCompositeMap("");
        }

        if (!mCompositeMap.isNull())
        {
            Ogre::TextureManager::getSingleton().remove(mCompositeMap->getName());
            mCompositeMap.setNull();
        }
    }
    else if (children && hasChildren())
        for (int i=0; i<4; ++i)
            mChildren[i]->destroyChunks(true);
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
    return mSceneNode->isInSceneGraph() && mChunk && mChunk->getVisible();
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
    std::vector<LayerInfo> layerList;
    mTerrain->getStorage()->getBlendmaps(mSize, mCenter, mTerrain->getShadersEnabled(), blendmaps, layerList);

    mMaterialGenerator->setLayerList(layerList);
    mMaterialGenerator->setBlendmapList(blendmaps);
}

void QuadTreeNode::prepareForCompositeMap(Ogre::TRect<float> area)
{
    Ogre::SceneManager* sceneMgr = mTerrain->getCompositeMapSceneManager();

    if (mIsDummy)
    {
        // TODO - store this default material somewhere instead of creating one for each empty cell
        MaterialGenerator matGen(mTerrain->getShadersEnabled());
        std::vector<LayerInfo> layer;
        LayerInfo info;
        info.mDiffuseMap = "textures\\_land_default.dds";
        info.mParallax = false;
        info.mSpecular = false;
        layer.push_back(info);
        matGen.setLayerList(layer);
        makeQuad(sceneMgr, area.left, area.top, area.right, area.bottom, matGen.generateForCompositeMapRTT(Ogre::MaterialPtr()));
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
            mChunk->setMaterial(mMaterialGenerator->generate(Ogre::MaterialPtr()));
        else
            mChunk->setMaterial(mMaterialGenerator->generateForCompositeMap(Ogre::MaterialPtr()));
    }
    if (hasChildren())
        for (int i=0; i<4; ++i)
            mChildren[i]->applyMaterials();
}

void QuadTreeNode::setVisible(bool visible)
{
    if (!visible && mChunk)
        mChunk->setVisible(false);

    if (hasChildren())
        for (int i=0; i<4; ++i)
            mChildren[i]->setVisible(visible);
}
