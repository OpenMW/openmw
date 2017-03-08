#include "quadtreeworld.hpp"

#include <components/sceneutil/workqueue.hpp>

#include "quadtreenode.hpp"
#include "storage.hpp"


#include <osg/io_utils>
#include <osg/Timer>

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

}

namespace Terrain
{


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

void QuadTreeWorld::accept(osg::NodeVisitor &nv)
{
    if (nv.getVisitorType() != osg::NodeVisitor::CULL_VISITOR && nv.getVisitorType() != osg::NodeVisitor::INTERSECTION_VISITOR)
        return;

    mRootNode->traverse(nv);
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
