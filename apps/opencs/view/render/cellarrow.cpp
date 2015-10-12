
#include "cellarrow.hpp"

#include <osg/Group>
#include <osg/PositionAttitudeTransform>

#include <osg/ShapeDrawable>
#include <osg/Shape>
#include <osg/Geode>


#include "elements.hpp"

CSVRender::CellArrowTag::CellArrowTag (CellArrow *arrow)
: TagBase (Element_CellArrow), mArrow (arrow)
{}

CSVRender::CellArrow *CSVRender::CellArrowTag::getCellArrow() const
{
    return mArrow;
}


void CSVRender::CellArrow::adjustTransform()
{
    // position
    const int cellSize = 8192;
    const int offset = cellSize / 2 + 400;

    int x = mXIndex*cellSize + cellSize/2;
    int y = mYIndex*cellSize + cellSize/2;

    switch (mDirection)
    {
        case Direction_North: y += offset; break;
        case Direction_West: x -= offset; break;
        case Direction_South: y -= offset; break;
        case Direction_East: x += offset; break;
    };

    mBaseNode->setPosition (osg::Vec3f (x, y, 0));

    // orientation
    osg::Quat xr (0, osg::Vec3f (1,0,0));
    osg::Quat yr (0, osg::Vec3f (0,1,0));
    osg::Quat zr (0, osg::Vec3f (0,0,1));
    mBaseNode->setAttitude (zr*yr*xr);
}

void CSVRender::CellArrow::buildShape()
{
    /// \todo placeholder shape -> replace
    osg::ref_ptr<osg::Box> shape(new osg::Box(osg::Vec3f(0,0,0), 200));
    osg::ref_ptr<osg::ShapeDrawable> shapedrawable(new osg::ShapeDrawable);
    shapedrawable->setShape(shape);

    osg::ref_ptr<osg::Geode> geode (new osg::Geode);
    geode->addDrawable(shapedrawable);

    mBaseNode->addChild (geode);
}

CSVRender::CellArrow::CellArrow (osg::Group *cellNode, Direction direction,
    int xIndex, int yIndex)
: mDirection (direction), mParentNode (cellNode), mXIndex (xIndex), mYIndex (yIndex)
{
    mBaseNode = new osg::PositionAttitudeTransform;

    mBaseNode->setUserData (new CellArrowTag (this));

    mParentNode->addChild (mBaseNode);

    // 0x1 reserved for separating cull and update visitors
    mBaseNode->setNodeMask (Element_CellArrow<<1);

    adjustTransform();
    buildShape();
}

CSVRender::CellArrow::~CellArrow()
{
    mParentNode->removeChild (mBaseNode);
}

int CSVRender::CellArrow::getXIndex() const
{
    return mXIndex;
}

int CSVRender::CellArrow::getYIndex() const
{
    return mYIndex;
}
