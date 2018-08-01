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
    return mNeighbours[dir];
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

    if ((mLodCallback && mLodCallback->isSufficientDetail(this, vd->getEyePoint())) || !getNumChildren())
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
        vd->setEyePoint(nv.getEyePoint());
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
