#include "quadtreeworld.hpp"

#include <osgUtil/CullVisitor>

#include <sstream>

#include <components/misc/constants.hpp>

#include "quadtreenode.hpp"
#include "storage.hpp"
#include "viewdata.hpp"
#include "chunkmanager.hpp"
#include "compositemaprenderer.hpp"

namespace
{

    bool isPowerOfTwo(int x)
    {
        return ( (x > 0) && ((x & (x - 1)) == 0) );
    }

    int nextPowerOfTwo (int v)
    {
        if (isPowerOfTwo(v)) return v;
        int depth=0;
        while(v)
        {
            v >>= 1;
            depth++;
        }
        return 1 << depth;
    }

    int Log2( unsigned int n )
    {
        int targetlevel = 0;
        while (n >>= 1) ++targetlevel;
        return targetlevel;
    }

}

namespace Terrain
{

class DefaultLodCallback : public LodCallback
{
public:
    DefaultLodCallback(float factor, float minSize)
        : mFactor(factor)
        , mMinSize(minSize)
    {
    }

    virtual bool isSufficientDetail(QuadTreeNode* node, float dist)
    {
        int nativeLodLevel = Log2(static_cast<unsigned int>(node->getSize()/mMinSize));
        int lodLevel = Log2(static_cast<unsigned int>((dist/mFactor)/(Constants::CellSizeInUnits*mMinSize)));

        return nativeLodLevel <= lodLevel;
    }

private:
    float mFactor;
    float mMinSize;
};

class RootNode : public QuadTreeNode
{
public:
    RootNode(float size, const osg::Vec2f& center)
        : QuadTreeNode(nullptr, Root, size, center)
        , mWorld(nullptr)
    {
    }

    void setWorld(QuadTreeWorld* world)
    {
        mWorld = world;
    }

    virtual void accept(osg::NodeVisitor &nv)
    {
        if (!nv.validNodeMask(*this))
            return;
        nv.pushOntoNodePath(this);
        mWorld->accept(nv);
        nv.popFromNodePath();
    }

private:
    QuadTreeWorld* mWorld;
};

class QuadTreeBuilder
{
public:
    QuadTreeBuilder(Terrain::Storage* storage, ViewDataMap* viewDataMap, float lodFactor, float minSize)
        : mStorage(storage)
        , mLodFactor(lodFactor)
        , mMinX(0.f), mMaxX(0.f), mMinY(0.f), mMaxY(0.f)
        , mMinSize(minSize)
        , mViewDataMap(viewDataMap)
    {
    }

    void build()
    {
        mStorage->getBounds(mMinX, mMaxX, mMinY, mMaxY);

        int origSizeX = static_cast<int>(mMaxX - mMinX);
        int origSizeY = static_cast<int>(mMaxY - mMinY);

        // Dividing a quad tree only works well for powers of two, so round up to the nearest one
        int size = nextPowerOfTwo(std::max(origSizeX, origSizeY));

        float centerX = (mMinX+mMaxX)/2.f + (size-origSizeX)/2.f;
        float centerY = (mMinY+mMaxY)/2.f + (size-origSizeY)/2.f;

        mRootNode = new RootNode(size, osg::Vec2f(centerX, centerY));
        mRootNode->setViewDataMap(mViewDataMap);
        mRootNode->setLodCallback(new DefaultLodCallback(mLodFactor, mMinSize));
        addChildren(mRootNode);

        mRootNode->initNeighbours();
    }

    void addChildren(QuadTreeNode* parent)
    {
        float halfSize = parent->getSize()/2.f;
        osg::BoundingBox boundingBox;
        for (unsigned int i=0; i<4; ++i)
        {
            QuadTreeNode* child = addChild(parent, static_cast<ChildDirection>(i), halfSize);
            if (child)
                boundingBox.expandBy(child->getBoundingBox());
        }

        parent->setBoundingBox(boundingBox);
    }

    QuadTreeNode* addChild(QuadTreeNode* parent, ChildDirection direction, float size)
    {
        osg::Vec2f center;
        switch (direction)
        {
        case SW:
            center = parent->getCenter() + osg::Vec2f(-size/2.f,-size/2.f);
            break;
        case SE:
            center = parent->getCenter() + osg::Vec2f(size/2.f, -size/2.f);
            break;
        case NW:
            center = parent->getCenter() + osg::Vec2f(-size/2.f, size/2.f);
            break;
        case NE:
            center = parent->getCenter() + osg::Vec2f(size/2.f, size/2.f);
            break;
        default:
            break;
        }

        osg::ref_ptr<QuadTreeNode> node = new QuadTreeNode(parent, direction, size, center);
        node->setLodCallback(parent->getLodCallback());
        node->setViewDataMap(mViewDataMap);
        parent->addChild(node);

        if (center.x() - size > mMaxX
                || center.x() + size < mMinX
                || center.y() - size > mMaxY
                || center.y() + size < mMinY )
            // Out of bounds of the actual terrain - this will happen because
            // we rounded the size up to the next power of two
        {
            // Still create and return an empty node so as to not break the assumption that each QuadTreeNode has either 4 or 0 children.
            return node;
        }

        if (node->getSize() <= mMinSize)
        {
            // We arrived at a leaf
            float minZ,maxZ;
            if (mStorage->getMinMaxHeights(size, center, minZ, maxZ))
            {
                float cellWorldSize = mStorage->getCellWorldSize();
                osg::BoundingBox boundingBox(osg::Vec3f((center.x()-size)*cellWorldSize, (center.y()-size)*cellWorldSize, minZ),
                                        osg::Vec3f((center.x()+size)*cellWorldSize, (center.y()+size)*cellWorldSize, maxZ));
                node->setBoundingBox(boundingBox);
            }
            return node;
        }
        else
        {
            addChildren(node);
            return node;
        }


        return node;
    }

    osg::ref_ptr<RootNode> getRootNode()
    {
        return mRootNode;
    }

private:
    Terrain::Storage* mStorage;

    float mLodFactor;
    float mMinX, mMaxX, mMinY, mMaxY;
    float mMinSize;
    ViewDataMap* mViewDataMap;

    osg::ref_ptr<RootNode> mRootNode;
};

QuadTreeWorld::QuadTreeWorld(osg::Group *parent, osg::Group *compileRoot, Resource::ResourceSystem *resourceSystem, Storage *storage, int nodeMask, int preCompileMask, int borderMask, int compositeMapResolution, float compositeMapLevel, float lodFactor, bool waitForCompositeMaps)
    : TerrainGrid(parent, compileRoot, resourceSystem, storage, nodeMask, preCompileMask, borderMask)
    , mViewDataMap(new ViewDataMap)
    , mQuadTreeBuilt(false)
    , mLodFactor(lodFactor)
    , mWaitForCompositeMaps(waitForCompositeMaps)
    , mViewDistance(std::numeric_limits<float>::max())
{
    // No need for culling on the Drawable / Transform level as the quad tree performs the culling already.
    mChunkManager->setCullingActive(false);

    mChunkManager->setCompositeMapSize(compositeMapResolution);
    mChunkManager->setCompositeMapLevel(compositeMapLevel);
}

QuadTreeWorld::~QuadTreeWorld()
{
    mViewDataMap->clear();
}


void traverse(QuadTreeNode* node, ViewData* vd, osg::NodeVisitor* nv, LodCallback* lodCallback, const osg::Vec3f& eyePoint, bool visible, float maxDist)
{
    if (!node->hasValidBounds())
        return;

    if (nv && nv->getVisitorType() == osg::NodeVisitor::CULL_VISITOR)
        visible = visible && !static_cast<osgUtil::CullVisitor*>(nv)->isCulled(node->getBoundingBox());

    float dist = node->distance(eyePoint);
    if (dist > maxDist)
        return;

    bool stopTraversal = (lodCallback && lodCallback->isSufficientDetail(node, dist)) || !node->getNumChildren();

    if (stopTraversal)
        vd->add(node, visible);
    else
    {
        for (unsigned int i=0; i<node->getNumChildren(); ++i)
            traverse(node->getChild(i), vd, nv, lodCallback, eyePoint, visible, maxDist);
    }
}

void traverseToCell(QuadTreeNode* node, ViewData* vd, int cellX, int cellY)
{
    if (!node->hasValidBounds())
        return;

    if (node->getCenter().x() + node->getSize()/2.f <= cellX
            || node->getCenter().x() - node->getSize()/2.f >= cellX+1
            || node->getCenter().y() + node->getSize()/2.f <= cellY
            || node->getCenter().y() - node->getSize()/2.f >= cellY+1)
        return;

    bool stopTraversal = (node->getSize() == 1);

    if (stopTraversal)
        vd->add(node, true);
    else
    {
        for (unsigned int i=0; i<node->getNumChildren(); ++i)
            traverseToCell(node->getChild(i), vd, cellX, cellY);
    }
}

unsigned int getLodFlags(QuadTreeNode* node, int ourLod, ViewData* vd)
{
    unsigned int lodFlags = 0;
    for (unsigned int i=0; i<4; ++i)
    {
        QuadTreeNode* neighbour = node->getNeighbour(static_cast<Direction>(i));

        // If the neighbour isn't currently rendering itself,
        // go up until we find one. NOTE: We don't need to go down,
        // because in that case neighbour's detail would be higher than
        // our detail and the neighbour would handle stitching by itself.
        while (neighbour && !vd->contains(neighbour))
            neighbour = neighbour->getParent();
        int lod = 0;
        if (neighbour)
            lod = Log2(int(neighbour->getSize()));

        if (lod <= ourLod) // We only need to worry about neighbours less detailed than we are -
            lod = 0;         // neighbours with more detail will do the stitching themselves
        // Use 4 bits for each LOD delta
        if (lod > 0)
        {
            lodFlags |= static_cast<unsigned int>(lod - ourLod) << (4*i);
        }
    }
    return lodFlags;
}

void loadRenderingNode(ViewData::Entry& entry, ViewData* vd, ChunkManager* chunkManager)
{
    if (vd->hasChanged())
    {
        // have to recompute the lodFlags in case a neighbour has changed LOD.
        int ourLod = Log2(int(entry.mNode->getSize()));
        unsigned int lodFlags = getLodFlags(entry.mNode, ourLod, vd);
        if (lodFlags != entry.mLodFlags)
        {
            entry.mRenderingNode = nullptr;
            entry.mLodFlags = lodFlags;
        }
    }

    if (!entry.mRenderingNode)
    {
        int ourLod = Log2(int(entry.mNode->getSize()));
        entry.mRenderingNode = chunkManager->getChunk(entry.mNode->getSize(), entry.mNode->getCenter(), ourLod, entry.mLodFlags);
    }
}

void QuadTreeWorld::accept(osg::NodeVisitor &nv)
{
    if (nv.getVisitorType() != osg::NodeVisitor::CULL_VISITOR && nv.getVisitorType() != osg::NodeVisitor::INTERSECTION_VISITOR)
        return;

    ViewData* vd = mRootNode->getView(nv);

    if (nv.getVisitorType() == osg::NodeVisitor::CULL_VISITOR)
    {
        osgUtil::CullVisitor* cv = static_cast<osgUtil::CullVisitor*>(&nv);

        osg::UserDataContainer* udc = cv->getCurrentCamera()->getUserDataContainer();
        if (udc && udc->getNumDescriptions() >= 2 && udc->getDescriptions()[0] == "NoTerrainLod")
        {
            std::istringstream stream(udc->getDescriptions()[1]);
            int x,y;
            stream >> x;
            stream >> y;
            traverseToCell(mRootNode.get(), vd, x,y);
        }
        else
            traverse(mRootNode.get(), vd, cv, mRootNode->getLodCallback(), cv->getViewPoint(), true, mViewDistance);
    }
    else
        mRootNode->traverse(nv);

    for (unsigned int i=0; i<vd->getNumEntries(); ++i)
    {
        ViewData::Entry& entry = vd->getEntry(i);

        loadRenderingNode(entry, vd, mChunkManager.get());

        if (entry.mVisible)
        {
            osg::UserDataContainer* udc = entry.mRenderingNode->getUserDataContainer();
            if (udc && udc->getUserData())
            {
                if (mWaitForCompositeMaps)
                    mCompositeMapRenderer->setImmediate(static_cast<CompositeMap*>(udc->getUserData()));
                udc->setUserData(nullptr);
            }
            entry.mRenderingNode->accept(nv);
        }
    }

    vd->reset(nv.getTraversalNumber());

    mRootNode->getViewDataMap()->clearUnusedViews(nv.getTraversalNumber());
}

void QuadTreeWorld::ensureQuadTreeBuilt()
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(mQuadTreeMutex);
    if (mQuadTreeBuilt)
        return;

    const float minSize = 1/8.f;
    QuadTreeBuilder builder(mStorage, mViewDataMap.get(), mLodFactor, minSize);
    builder.build();

    mRootNode = builder.getRootNode();
    mRootNode->setWorld(this);
    mQuadTreeBuilt = true;
}

void QuadTreeWorld::enable(bool enabled)
{
    if (enabled)
    {
        ensureQuadTreeBuilt();

        if (!mRootNode->getNumParents())
            mTerrainRoot->addChild(mRootNode);
    }

    if (mRootNode)
        mRootNode->setNodeMask(enabled ? ~0 : 0);
}

void QuadTreeWorld::cacheCell(View *view, int x, int y)
{
    ensureQuadTreeBuilt();
    ViewData* vd = static_cast<ViewData*>(view);
    traverseToCell(mRootNode.get(), vd, x, y);

    for (unsigned int i=0; i<vd->getNumEntries(); ++i)
    {
        ViewData::Entry& entry = vd->getEntry(i);
        loadRenderingNode(entry, vd, mChunkManager.get());
    }
}

View* QuadTreeWorld::createView()
{
    return new ViewData;
}

void QuadTreeWorld::preload(View *view, const osg::Vec3f &eyePoint, volatile bool &abort)
{
    ensureQuadTreeBuilt();

    ViewData* vd = static_cast<ViewData*>(view);
    traverse(mRootNode.get(), vd, nullptr, mRootNode->getLodCallback(), eyePoint, false, mViewDistance);

    for (unsigned int i=0; i<vd->getNumEntries() && !abort; ++i)
    {
        ViewData::Entry& entry = vd->getEntry(i);
        loadRenderingNode(entry, vd, mChunkManager.get());
    }
}

void QuadTreeWorld::reportStats(unsigned int frameNumber, osg::Stats *stats)
{
    stats->setAttribute(frameNumber, "Composite", mCompositeMapRenderer->getCompileSetSize());
}

void QuadTreeWorld::setDefaultViewer(osg::Object *obj)
{
    mViewDataMap->setDefaultViewer(obj);
}

void QuadTreeWorld::loadCell(int x, int y)
{
    // fallback behavior only for undefined cells (every other is already handled in quadtree)
    float dummy;
    if (!mStorage->getMinMaxHeights(1, osg::Vec2f(x+0.5, y+0.5), dummy, dummy))
        TerrainGrid::loadCell(x,y);
    else
        World::loadCell(x,y);
}

void QuadTreeWorld::unloadCell(int x, int y)
{
    // fallback behavior only for undefined cells (every other is already handled in quadtree)
    float dummy;
    if (!mStorage->getMinMaxHeights(1, osg::Vec2f(x+0.5, y+0.5), dummy, dummy))
        TerrainGrid::unloadCell(x,y);
    else
        World::unloadCell(x,y);
}


}
