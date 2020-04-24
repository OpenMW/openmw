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
        return nullptr; // Arrived at root node, the root node does not have neighbours

    QuadTreeNode* nextNode;
    if (adjacent(currentNode->getDirection(), dir))
        nextNode = searchNeighbour(currentNode->getParent(), dir);
    else
        nextNode = currentNode->getParent();

    if (nextNode && nextNode->getNumChildren())
        return nextNode->getChild(reflect(currentNode->getDirection(), dir));
    else
        return nullptr;
}

QuadTreeNode::QuadTreeNode(QuadTreeNode* parent, ChildDirection direction, float size, const osg::Vec2f& center)
    : mParent(parent)
    , mDirection(direction)
    , mValidBounds(false)
    , mSize(size)
    , mCenter(center)
{
    for (unsigned int i=0; i<4; ++i)
        mNeighbours[i] = 0;
}

QuadTreeNode::~QuadTreeNode()
{
}

QuadTreeNode *QuadTreeNode::getNeighbour(Direction dir)
{
    return mNeighbours[dir];
}

float QuadTreeNode::distance(const osg::Vec3f& v) const
{
    const osg::BoundingBox& box = getBoundingBox();
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

void QuadTreeNode::initNeighbours()
{
    for (int i=0; i<4; ++i)
        mNeighbours[i] = searchNeighbour(this, (Direction)i);

    for (unsigned int i=0; i<getNumChildren(); ++i)
        getChild(i)->initNeighbours();
}

void QuadTreeNode::traverseNodes(ViewData* vd, const osg::Vec3f& viewPoint, LodCallback* lodCallback, float maxDist)
{
    if (!hasValidBounds())
        return;

    float dist = distance(viewPoint);
    if (dist > maxDist)
        return;

    bool stopTraversal = (lodCallback->isSufficientDetail(this, dist)) || !getNumChildren();

    if (stopTraversal)
        vd->add(this);
    else
    {
        for (unsigned int i=0; i<getNumChildren(); ++i)
            getChild(i)->traverseNodes(vd, viewPoint, lodCallback, maxDist);
    }
}

void QuadTreeNode::traverseTo(ViewData* vd, float size, const osg::Vec2f& center)
{
    if (!hasValidBounds())
        return;

    if (getCenter().x() + getSize()/2.f <= center.x() - size/2.f
            || getCenter().x() - getSize()/2.f >= center.x() + size/2.f
            || getCenter().y() + getSize()/2.f <= center.y() - size/2.f
            || getCenter().y() - getSize()/2.f >= center.y() + size/2.f)
        return;

    bool stopTraversal = (getSize() == size);

    if (stopTraversal)
        vd->add(this);
    else
    {
        for (unsigned int i=0; i<getNumChildren(); ++i)
            getChild(i)->traverseTo(vd, size, center);
    }
}

void QuadTreeNode::intersect(ViewData* vd, TerrainLineIntersector& intersector)
{
    if (!hasValidBounds())
        return;

    if (!intersector.intersectAndClip(getBoundingBox()))
        return;

    if (getNumChildren() == 0)
        vd->add(this);
    else
    {
        for (unsigned int i=0; i<getNumChildren(); ++i)
            getChild(i)->intersect(vd, intersector);
    }
}

void QuadTreeNode::setBoundingBox(const osg::BoundingBox &boundingBox)
{
    mBoundingBox = boundingBox;
    mValidBounds = boundingBox.valid();
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
