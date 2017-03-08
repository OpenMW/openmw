#include "quadtreeworld.hpp"

#include <osgUtil/CullVisitor>

#include <components/sceneutil/workqueue.hpp>

#include "quadtreenode.hpp"
#include "storage.hpp"
#include "viewdata.hpp"
#include "chunkmanager.hpp"

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

    float distance(const osg::BoundingBox& box, const osg::Vec3f& v)
    {
        if (box.contains(v))
            return 0;
        else
        {
            osg::Vec3f maxDist(0,0,0);

            if (v.x() < box.xMin())
                maxDist.x() = box.xMin() - v.x();
            else if (v.x() > box.xMax())
                maxDist.x() = v.x() - box.xMax();

            if (v.y() < box.yMin())
                maxDist.y() = box.yMin() - v.y();
            else if (v.y() > box.yMax())
                maxDist.y() = v.y() - box.yMax();

            if (v.z() < box.zMin())
                maxDist.z() = box.zMin() - v.z();
            else if (v.z() > box.zMax())
                maxDist.z() = v.z() - box.zMax();

            return maxDist.length();
        }
    }

}

namespace Terrain
{

class DefaultLodCallback : public LodCallback
{
public:
    virtual bool isSufficientDetail(QuadTreeNode* node, osg::NodeVisitor& nv)
    {
        float dist = distance(node->getBoundingBox(), nv.getEyePoint());
        int nativeLodLevel = Log2(static_cast<unsigned int>(node->getSize()*4));
        int lodLevel = Log2(static_cast<unsigned int>(dist/2048.0));

        return nativeLodLevel <= lodLevel;
    }
};

class RootNode : public QuadTreeNode
{
public:
    RootNode(float size, const osg::Vec2f& center)
        : QuadTreeNode(NULL, Root, size, center)
        , mWorld(NULL)
    {
    }

    void setWorld(QuadTreeWorld* world)
    {
        mWorld = world;
    }

    virtual void accept(osg::NodeVisitor &nv)
    {
        nv.pushOntoNodePath(this);
        mWorld->accept(nv);
        nv.popFromNodePath();
    }

private:
    QuadTreeWorld* mWorld;
};

class BuildQuadTreeItem : public SceneUtil::WorkItem
{
public:
    BuildQuadTreeItem(Terrain::Storage* storage, float minSize)
        : mStorage(storage)
        , mMinX(0.f), mMaxX(0.f), mMinY(0.f), mMaxY(0.f)
        , mMinSize(minSize)
    {
    }

    virtual void doWork()
    {
        mStorage->getBounds(mMinX, mMaxX, mMinY, mMaxY);

        int origSizeX = static_cast<int>(mMaxX - mMinX);
        int origSizeY = static_cast<int>(mMaxY - mMinY);

        // Dividing a quad tree only works well for powers of two, so round up to the nearest one
        int size = nextPowerOfTwo(std::max(origSizeX, origSizeY));

        float centerX = (mMinX+mMaxX)/2.f + (size-origSizeX)/2.f;
        float centerY = (mMinY+mMaxY)/2.f + (size-origSizeY)/2.f;

        mRootNode = new RootNode(size, osg::Vec2f(centerX, centerY));
        mRootNode->setViewDataMap(new ViewDataMap);
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
        node->setLodCallback(new DefaultLodCallback);
        node->setViewDataMap(parent->getViewDataMap());
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
    }

    osg::ref_ptr<RootNode> getRootNode()
    {
        return mRootNode;
    }

private:
    Terrain::Storage* mStorage;

    float mMinX, mMaxX, mMinY, mMaxY;
    float mMinSize;

    osg::ref_ptr<RootNode> mRootNode;
};

QuadTreeWorld::QuadTreeWorld(SceneUtil::WorkQueue* workQueue, osg::Group *parent, osg::Group *compileRoot, Resource::ResourceSystem *resourceSystem, Storage *storage, int nodeMask, int preCompileMask)
    : World(parent, compileRoot, resourceSystem, storage, nodeMask, preCompileMask)
    , mWorkQueue(workQueue)
    , mQuadTreeBuilt(false)
{
}

QuadTreeWorld::~QuadTreeWorld()
{
    if (mWorkItem)
    {
        mWorkItem->abort();
        mWorkItem->waitTillDone();
    }
}


void traverse(QuadTreeNode* node, ViewData* vd, osgUtil::CullVisitor* cv, bool visible)
{
    if (!node->hasValidBounds())
        return;

    visible = visible && !cv->isCulled(node->getBoundingBox());
    bool stopTraversal = (node->getLodCallback() && node->getLodCallback()->isSufficientDetail(node, *cv)) || !node->getNumChildren();

    if (stopTraversal)
        vd->add(node, visible);
    else
    {
        for (unsigned int i=0; i<node->getNumChildren(); ++i)
            traverse(node->getChild(i), vd, cv, visible);
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

void QuadTreeWorld::accept(osg::NodeVisitor &nv)
{
    if (nv.getVisitorType() != osg::NodeVisitor::CULL_VISITOR)// && nv.getVisitorType() != osg::NodeVisitor::INTERSECTION_VISITOR)
        return;

    ViewData* vd = mRootNode->getView(nv);

    if (nv.getVisitorType() == osg::NodeVisitor::CULL_VISITOR)
    {
        osgUtil::CullVisitor* cv = static_cast<osgUtil::CullVisitor*>(&nv);
        traverse(mRootNode.get(), vd, cv, true);
    }
    else
        mRootNode->traverse(nv);

    for (unsigned int i=0; i<vd->getNumEntries(); ++i)
    {
        ViewData::Entry& entry = vd->getEntry(i);
        if (!entry.mRenderingNode)
        {
            int ourLod = Log2(int(entry.mNode->getSize()));
            unsigned int lodFlags = getLodFlags(entry.mNode, ourLod, vd);
            entry.mRenderingNode = mChunkManager->getChunk(entry.mNode->getSize(), entry.mNode->getCenter(), ourLod, lodFlags);
        }

        if (entry.mVisible)
            entry.mRenderingNode->accept(nv);
    }

    vd->reset(nv.getTraversalNumber());

    mRootNode->getViewDataMap()->clearUnusedViews(nv.getTraversalNumber());
}

void QuadTreeWorld::loadCell(int x, int y)
{
    if (mQuadTreeBuilt && !mRootNode)
    {
        mRootNode = mWorkItem->getRootNode();
        mRootNode->setWorld(this);
        mTerrainRoot->addChild(mRootNode);
        mWorkItem = NULL;
    }
}

osg::ref_ptr<osg::Node> QuadTreeWorld::cacheCell(int x, int y)
{
    if (!mQuadTreeBuilt)
    {
        const float minSize = 1/4.f;
        mWorkItem = new BuildQuadTreeItem(mStorage, minSize);
        mWorkQueue->addWorkItem(mWorkItem);

        mWorkItem->waitTillDone();

        mQuadTreeBuilt = true;
    }
    return NULL;
}



}
