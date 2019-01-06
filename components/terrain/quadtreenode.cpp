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
}

QuadTreeNode::~QuadTreeNode()
{
}

QuadTreeNode *QuadTreeNode::getNeighbour(Direction dir)
{
    return searchNeighbour(this, dir);
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

void QuadTreeNode::traverse(ViewData* vd, const osg::Vec3f& viewPoint, LodCallback* lodCallback, float maxDist)
{
    if (!hasValidBounds())
        return;

    float dist = distance(viewPoint);
    if (dist > maxDist)
        return;

    bool stopTraversal = (lodCallback->isSufficientDetail(this, dist)) || !getNumChildren();

    if (stopTraversal)
        vd->add(this, true);
    else
    {
        for (unsigned int i=0; i<getNumChildren(); ++i)
            getChild(i)->traverse(vd, viewPoint, lodCallback, maxDist);
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
        vd->add(this, true);
    else
    {
        for (unsigned int i=0; i<getNumChildren(); ++i)
            getChild(i)->traverseTo(vd, size, center);
    }
}

// copied from LineSegmentIntersector::intersectAndClip (protected function)
bool intersectAndClip(osg::Vec3f& s, osg::Vec3f& e,const osg::BoundingBox& bbInput)
{
    osg::Vec3f bb_min(bbInput._min);
    osg::Vec3f bb_max(bbInput._max);

    double epsilon = 1e-5;

    // compate s and e against the xMin to xMax range of bb.
    if (s.x()<=e.x())
    {
        // trivial reject of segment wholely outside.
        if (e.x()<bb_min.x()) return false;
        if (s.x()>bb_max.x()) return false;

        if (s.x()<bb_min.x())
        {
            // clip s to xMin.
            double r = (bb_min.x()-s.x())/(e.x()-s.x()) - epsilon;
            if (r>0.0) s = s + (e-s)*r;
        }

        if (e.x()>bb_max.x())
        {
            // clip e to xMax.
            double r = (bb_max.x()-s.x())/(e.x()-s.x()) + epsilon;
            if (r<1.0) e = s+(e-s)*r;
        }
    }
    else
    {
        if (s.x()<bb_min.x()) return false;
        if (e.x()>bb_max.x()) return false;

        if (e.x()<bb_min.x())
        {
            // clip e to xMin.
            double r = (bb_min.x()-e.x())/(s.x()-e.x()) - epsilon;
            if (r>0.0) e = e + (s-e)*r;
        }

        if (s.x()>bb_max.x())
        {
            // clip s to xMax.
            double r = (bb_max.x()-e.x())/(s.x()-e.x()) + epsilon;
            if (r<1.0) s = e + (s-e)*r;
        }
    }

    // compate s and e against the yMin to yMax range of bb.
    if (s.y()<=e.y())
    {
        // trivial reject of segment wholely outside.
        if (e.y()<bb_min.y()) return false;
        if (s.y()>bb_max.y()) return false;

        if (s.y()<bb_min.y())
        {
            // clip s to yMin.
            double r = (bb_min.y()-s.y())/(e.y()-s.y()) - epsilon;
            if (r>0.0) s = s + (e-s)*r;
        }

        if (e.y()>bb_max.y())
        {
            // clip e to yMax.
            double r = (bb_max.y()-s.y())/(e.y()-s.y()) + epsilon;
            if (r<1.0) e = s+(e-s)*r;
        }
    }
    else
    {
        if (s.y()<bb_min.y()) return false;
        if (e.y()>bb_max.y()) return false;

        if (e.y()<bb_min.y())
        {
            // clip e to yMin.
            double r = (bb_min.y()-e.y())/(s.y()-e.y()) - epsilon;
            if (r>0.0) e = e + (s-e)*r;
        }

        if (s.y()>bb_max.y())
        {
            // clip s to yMax.
            double r = (bb_max.y()-e.y())/(s.y()-e.y()) + epsilon;
            if (r<1.0) s = e + (s-e)*r;
        }
    }

    return true;
}


void QuadTreeNode::intersect(ViewData* vd, const osg::Vec3f& start, const osg::Vec3f& end)
{
    if (!hasValidBounds())
        return;

    osg::Vec3f s = start;
    osg::Vec3f e = end;
    if (!intersectAndClip(s, e, getBoundingBox()))
        return;

    if (getNumChildren() == 0)
        vd->add(this, true);
    else
    {
        for (unsigned int i=0; i<getNumChildren(); ++i)
            getChild(i)->intersect(vd, start, end);
    }
}

void QuadTreeNode::setBoundingBox(const osg::BoundingBox &boundingBox)
{
    mBoundingBox = boundingBox;
    mValidBounds = boundingBox.valid();
    dirtyBound();
    getBound();
}

osg::BoundingSphere QuadTreeNode::computeBound() const
{
    return osg::BoundingSphere(mBoundingBox);
}

}
