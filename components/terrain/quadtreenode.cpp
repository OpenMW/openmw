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
    , mLoadState(LS_Unloaded)
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

void QuadTreeNode::update(const Ogre::Vector3 &cameraPos)
{
    const Ogre::AxisAlignedBox& bounds = getBoundingBox();
    if (bounds.isNull())
        return;

    float dist = distance(mWorldBounds, cameraPos);

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
    else if (dist > cellWorldSize)
        wantedLod = 1;

    mIsActive = true;

    bool wantToDisplay = mSize <= mTerrain->getMaxBatchSize() && mLodLevel <= wantedLod;

    if (wantToDisplay)
    {
        // Wanted LOD is small enough to render this node in one chunk
        if (mLoadState == LS_Unloaded)
        {
            mLoadState = LS_Loading;
            mTerrain->queueLoad(this);
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
                // Make sure child scene nodes are detached
                mSceneNode->removeAllChildren();

                // TODO: unload
                //for (int i=0; i<4; ++i)
                //    mChildren[i]->unload();
            }

            mChunk->setVisible(true);
        }
    }

    if (wantToDisplay && mLoadState != LS_Loaded)
    {
        // We want to display, but aren't ready yet. Perhaps our child nodes are ready?
        // TODO: this doesn't check child-child nodes...
        if (hasChildren())
        {
            for (int i=0; i<4; ++i)
                if (mChildren[i]->hasChunk())
                    mChildren[i]->update(cameraPos);
        }
    }
    if (!wantToDisplay)
    {
        // We do not want to display this node - delegate to children
        if (mChunk)
            mChunk->setVisible(false);
        if (hasChildren())
        {
            for (int i=0; i<4; ++i)
                mChildren[i]->update(cameraPos);
        }
    }
}

void QuadTreeNode::load(const LoadResponseData &data)
{
    assert (!mChunk);

    std::cout << "loading " << std::endl;
    mChunk = new Chunk(this, data);
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

    mChunk->setVisible(false);

    mLoadState = LS_Loaded;
}

void QuadTreeNode::unload()
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
    mLoadState = LS_Unloaded;
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
        layer.push_back(mTerrain->getStorage()->getDefaultLayer());
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
