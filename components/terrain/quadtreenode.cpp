#include "quadtreenode.hpp"

#include <cassert>

#include <osgUtil/CullVisitor>

#include "defs.hpp"
#include "viewdata.hpp"

namespace Terrain
{

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

QuadTreeNode* searchNeighbour (QuadTreeNode* currentNode, Direction dir)
{
    if (currentNode->getDirection() == Root)
        return NULL; // Arrived at root node, the root node does not have neighbours

    QuadTreeNode* nextNode;
    if (adjacent(currentNode->getDirection(), dir))
        nextNode = searchNeighbour(currentNode->getParent(), dir);
    else
        nextNode = currentNode->getParent();

    if (nextNode && nextNode->getNumChildren())
        return nextNode->getChild(reflect(currentNode->getDirection(), dir));
    else
        return NULL;
}

QuadTreeNode::QuadTreeNode(QuadTreeNode* parent, ChildDirection direction, float size, const osg::Vec2f& center)
    : mParent(parent)
    , mDirection(direction)
    , mSize(size)
    , mCenter(center)
{
    for (unsigned int i=0; i<4; ++i)
        mNeighbours[i] = 0;
}

QuadTreeNode::~QuadTreeNode()
{
}

QuadTreeNode* QuadTreeNode::getParent()
{
    return mParent;
}

QuadTreeNode *QuadTreeNode::getChild(unsigned int i)
{
    return static_cast<QuadTreeNode*>(Group::getChild(i));
}

void QuadTreeNode::initNeighbours()
{
    for (int i=0; i<4; ++i)
        mNeighbours[i] = searchNeighbour(this, (Direction)i);

    for (unsigned int i=0; i<getNumChildren(); ++i)
        getChild(i)->initNeighbours();
}

void QuadTreeNode::traverse(osg::NodeVisitor &nv)
{
    if (nv.getVisitorType() == osg::NodeVisitor::CULL_VISITOR)
    {
        osgUtil::CullVisitor* cv = static_cast<osgUtil::CullVisitor*>(&nv);

        // do another culling test against bounding box as its much more accurate than the bounding sphere.
        if (cv->isCulled(mBoundingBox))
            return;
    }

    if ((mLodCallback && mLodCallback->isSufficientDetail(this, nv)) || !getNumChildren())
        getView(nv)->add(this);
    else
        osg::Group::traverse(nv);
}

void QuadTreeNode::setLodCallback(LodCallback *lodCallback)
{
    mLodCallback = lodCallback;
}

void QuadTreeNode::setViewDataMap(ViewDataMap *map)
{
    mViewDataMap = map;
}

ViewDataMap *QuadTreeNode::getViewDataMap()
{
    return mViewDataMap;
}

ViewData* QuadTreeNode::getView(osg::NodeVisitor &nv)
{
    if (nv.getVisitorType() == osg::NodeVisitor::CULL_VISITOR)
    {
        osgUtil::CullVisitor* cv = static_cast<osgUtil::CullVisitor*>(&nv);
        return mViewDataMap->getViewData(cv->getCurrentCamera(), true);
    }
    else // INTERSECTION_VISITOR
    {
        return mViewDataMap->getViewData(&nv, false);
    }
}

void QuadTreeNode::setBoundingBox(const osg::BoundingBox &boundingBox)
{
    mBoundingBox = boundingBox;
    dirtyBound();
    getBound();
}

const osg::BoundingBox &QuadTreeNode::getBoundingBox() const
{
    return mBoundingBox;
}

osg::BoundingSphere QuadTreeNode::computeBound() const
{
    return osg::BoundingSphere(mBoundingBox);
}

float QuadTreeNode::getSize() const
{
    return mSize;
}

const osg::Vec2f &QuadTreeNode::getCenter() const
{
    return mCenter;
}

}
