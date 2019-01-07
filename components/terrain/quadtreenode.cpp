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
    , mViewDataMap(nullptr)
{
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

void QuadTreeNode::traverse(ViewData* vd, osg::NodeVisitor* nv, const osg::Vec3f& viewPoint, bool visible, float maxDist)
{
    if (!hasValidBounds())
        return;

    if (nv && nv->getVisitorType() == osg::NodeVisitor::CULL_VISITOR)
        visible = visible && !static_cast<osgUtil::CullVisitor*>(nv)->isCulled(mBoundingBox);

    float dist = distance(viewPoint);
    if (dist > maxDist)
        return;

    bool stopTraversal = (mLodCallback->isSufficientDetail(this, dist)) || !getNumChildren();

    if (stopTraversal)
        vd->add(this, visible);
    else
    {
        for (unsigned int i=0; i<getNumChildren(); ++i)
            getChild(i)->traverse(vd, nv, viewPoint, visible, maxDist);
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

void QuadTreeNode::traverse(osg::NodeVisitor &nv)
{
    if (!hasValidBounds())
        return;

    bool needsUpdate = true;
    ViewData* vd = getView(nv, needsUpdate);

    if ((mLodCallback && mLodCallback->isSufficientDetail(this, distance(vd->getViewPoint()))) || !getNumChildren())
        vd->add(this, true);
    else
        osg::Group::traverse(nv);
}

void QuadTreeNode::setLodCallback(LodCallback *lodCallback)
{
    mLodCallback = lodCallback;
}

LodCallback *QuadTreeNode::getLodCallback()
{
    return mLodCallback;
}

void QuadTreeNode::setViewDataMap(ViewDataMap *map)
{
    mViewDataMap = map;
}

ViewDataMap *QuadTreeNode::getViewDataMap()
{
    return mViewDataMap;
}

ViewData* QuadTreeNode::getView(osg::NodeVisitor &nv, bool& needsUpdate)
{
    ViewData* vd = NULL;
    if (nv.getVisitorType() == osg::NodeVisitor::CULL_VISITOR)
    {
        osgUtil::CullVisitor* cv = static_cast<osgUtil::CullVisitor*>(&nv);
        vd = mViewDataMap->getViewData(cv->getCurrentCamera(), nv.getViewPoint(), needsUpdate);
    }
    else // INTERSECTION_VISITOR
    {
        osg::Vec3f viewPoint = nv.getViewPoint();
        mViewDataMap->getDefaultViewPoint(viewPoint);

        static osg::ref_ptr<osg::Object> dummyObj = new osg::DummyObject;
        vd = mViewDataMap->getViewData(dummyObj.get(), viewPoint, needsUpdate);
        needsUpdate = true;
    }
    return vd;
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
