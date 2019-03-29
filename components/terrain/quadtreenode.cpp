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

void QuadTreeNode::traverse(osg::NodeVisitor &nv)
{
    if (!hasValidBounds())
        return;

    ViewData* vd = getView(nv);

    if ((mLodCallback && mLodCallback->isSufficientDetail(this, distance(vd->getEyePoint()))) || !getNumChildren())
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

ViewData* QuadTreeNode::getView(osg::NodeVisitor &nv)
{
    if (nv.getVisitorType() == osg::NodeVisitor::CULL_VISITOR)
    {
        osgUtil::CullVisitor* cv = static_cast<osgUtil::CullVisitor*>(&nv);
        ViewData* vd = mViewDataMap->getViewData(cv->getCurrentCamera());
        vd->setEyePoint(nv.getViewPoint());
        return vd;
    }
    else // INTERSECTION_VISITOR
    {
        static osg::ref_ptr<osg::Object> dummyObj = new osg::DummyObject;
        ViewData* vd = mViewDataMap->getViewData(dummyObj.get());
        ViewData* defaultView = mViewDataMap->getDefaultView();
        if (defaultView->hasEyePoint())
            vd->setEyePoint(defaultView->getEyePoint());
        else
            vd->setEyePoint(nv.getEyePoint());
        return vd;
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
