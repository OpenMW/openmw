#include "brushdraw.hpp"

#include <limits>

#include <osg/Group>
#include <osg/Geometry>
#include <osg/Array>

#include <osgUtil/LineSegmentIntersector>

#include "../../model/world/cellcoordinates.hpp"
#include "../widget/brushshapes.hpp"
#include "mask.hpp"

CSVRender::BrushDraw::BrushDraw(osg::ref_ptr<osg::Group> parentNode, bool textureMode) :
    mParentNode(parentNode), mTextureMode(textureMode)
{
    mBrushDrawNode = new osg::Group();
    mGeometry = new osg::Geometry();
    mBrushDrawNode->addChild(mGeometry);
    mParentNode->addChild(mBrushDrawNode);
    if (mTextureMode)
        mLandSizeFactor = static_cast<float>(ESM::Land::REAL_SIZE) / static_cast<float>(ESM::Land::LAND_TEXTURE_SIZE);
    else
        mLandSizeFactor = static_cast<float>(ESM::Land::REAL_SIZE) / static_cast<float>(ESM::Land::LAND_SIZE - 1);
}

CSVRender::BrushDraw::~BrushDraw()
{
    mBrushDrawNode->removeChild(mGeometry);
    mParentNode->removeChild(mBrushDrawNode);
}

float CSVRender::BrushDraw::getIntersectionHeight (const osg::Vec3d& point)
{
    osg::Vec3d start = point;
    osg::Vec3d end = point;
    start.z() = std::numeric_limits<int>::max();
    end.z() = std::numeric_limits<int>::lowest();
    osg::Vec3d direction = end - start;

    // Get intersection
    osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector (new osgUtil::LineSegmentIntersector(
        osgUtil::Intersector::MODEL, start, end) );
    intersector->setIntersectionLimit(osgUtil::LineSegmentIntersector::NO_LIMIT);
    osgUtil::IntersectionVisitor visitor(intersector);

    visitor.setTraversalMask(Mask_Terrain);

    mParentNode->accept(visitor);

    for (osgUtil::LineSegmentIntersector::Intersections::iterator it = intersector->getIntersections().begin();
         it != intersector->getIntersections().end(); ++it)
    {
        osgUtil::LineSegmentIntersector::Intersection intersection = *it;

        // reject back-facing polygons
        if (direction * intersection.getWorldIntersectNormal() > 0)
        {
            continue;
        }

        return intersection.getWorldIntersectPoint().z();
    }
    return 0.0f;
}

void CSVRender::BrushDraw::buildPointGeometry(const osg::Vec3d& point)
{
    osg::ref_ptr<osg::Geometry> geom (new osg::Geometry());
    osg::ref_ptr<osg::Vec3Array> vertices (new osg::Vec3Array());
    osg::ref_ptr<osg::Vec4Array> colors (new osg::Vec4Array());
    const float brushOutlineHeight (1.0f);
    const float crossHeadSize (8.0f);
    osg::Vec4f lineColor(1.0f, 1.0f, 1.0f, 0.6f);

    vertices->push_back(osg::Vec3d(
        point.x() - crossHeadSize,
        point.y() - crossHeadSize,
        getIntersectionHeight(osg::Vec3d(
            point.x() - crossHeadSize,
            point.y() - crossHeadSize,
            point.z()) ) + brushOutlineHeight));
    colors->push_back(lineColor);
    vertices->push_back(osg::Vec3d(
        point.x() + crossHeadSize,
        point.y() + crossHeadSize,
        getIntersectionHeight(osg::Vec3d(
            point.x() + crossHeadSize,
            point.y() + crossHeadSize,
            point.z()) ) + brushOutlineHeight));
    colors->push_back(lineColor);
    vertices->push_back(osg::Vec3d(
        point.x() + crossHeadSize,
        point.y() - crossHeadSize,
        getIntersectionHeight(osg::Vec3d(
            point.x() + crossHeadSize,
            point.y() - crossHeadSize,
            point.z()) ) + brushOutlineHeight));
    colors->push_back(lineColor);
    vertices->push_back(osg::Vec3d(
        point.x() - crossHeadSize,
        point.y() + crossHeadSize,
        getIntersectionHeight(osg::Vec3d(
            point.x() - crossHeadSize,
            point.y() + crossHeadSize,
            point.z()) ) + brushOutlineHeight));
    colors->push_back(lineColor);

    geom->setVertexArray(vertices);
    geom->setColorArray(colors, osg::Array::BIND_PER_VERTEX);
    geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINES, 0, 4));
    mGeometry = geom;
}

void CSVRender::BrushDraw::buildSquareGeometry(const float& radius, const osg::Vec3d& point)
{
    osg::ref_ptr<osg::Geometry> geom (new osg::Geometry());
    osg::ref_ptr<osg::Vec3Array> vertices (new osg::Vec3Array());
    osg::ref_ptr<osg::Vec4Array> colors (new osg::Vec4Array());

    const float brushOutlineHeight (1.0f);
    float diameter = radius * 2;
    int resolution = static_cast<int>(2.f * diameter / mLandSizeFactor); //half a vertex resolution
    float resAdjustedLandSizeFactor = mLandSizeFactor / 2;
    osg::Vec4f lineColor(1.0f, 1.0f, 1.0f, 0.6f);

    for (int i = 0; i < resolution; i++)
    {
        int step = i * resAdjustedLandSizeFactor;
        int step2 = (i + 1) * resAdjustedLandSizeFactor;

        osg::Vec3d upHorizontalLinePoint1(
            point.x() - radius + step,
            point.y() - radius,
            getIntersectionHeight(osg::Vec3d(
                point.x() - radius + step,
                point.y() - radius,
                point.z())) + brushOutlineHeight);
        osg::Vec3d upHorizontalLinePoint2(
            point.x() - radius + step2,
            point.y() - radius,
            getIntersectionHeight(osg::Vec3d(
                point.x() - radius + step2,
                point.y() - radius,
                point.z())) + brushOutlineHeight);
        osg::Vec3d upVerticalLinePoint1(
            point.x() - radius,
            point.y() - radius + step,
            getIntersectionHeight(osg::Vec3d(
                point.x() - radius,
                point.y() - radius + step,
                point.z())) + brushOutlineHeight);
        osg::Vec3d upVerticalLinePoint2(
            point.x() - radius,
            point.y() - radius + step2,
            getIntersectionHeight(osg::Vec3d(
                point.x() - radius,
                point.y() - radius + step2,
                point.z())) + brushOutlineHeight);
        osg::Vec3d downHorizontalLinePoint1(
            point.x() + radius - step,
            point.y() + radius,
            getIntersectionHeight(osg::Vec3d(
                point.x() + radius - step,
                point.y() + radius,
                point.z())) + brushOutlineHeight);
        osg::Vec3d downHorizontalLinePoint2(
            point.x() + radius - step2,
            point.y() + radius,
            getIntersectionHeight(osg::Vec3d(
                point.x() + radius - step2,
                point.y() + radius,
                point.z())) + brushOutlineHeight);
        osg::Vec3d downVerticalLinePoint1(
            point.x() + radius,
            point.y() + radius - step,
            getIntersectionHeight(osg::Vec3d(
                point.x() + radius,
                point.y() + radius - step,
                point.z())) + brushOutlineHeight);
        osg::Vec3d downVerticalLinePoint2(
            point.x() + radius,
            point.y() + radius - step2,
            getIntersectionHeight(osg::Vec3d(
                point.x() + radius,
                point.y() + radius - step2,
                point.z())) + brushOutlineHeight);
        vertices->push_back(upHorizontalLinePoint1);
        colors->push_back(lineColor);
        vertices->push_back(upHorizontalLinePoint2);
        colors->push_back(lineColor);
        vertices->push_back(upVerticalLinePoint1);
        colors->push_back(lineColor);
        vertices->push_back(upVerticalLinePoint2);
        colors->push_back(lineColor);
        vertices->push_back(downHorizontalLinePoint1);
        colors->push_back(lineColor);
        vertices->push_back(downHorizontalLinePoint2);
        colors->push_back(lineColor);
        vertices->push_back(downVerticalLinePoint1);
        colors->push_back(lineColor);
        vertices->push_back(downVerticalLinePoint2);
        colors->push_back(lineColor);
    }

    geom->setVertexArray(vertices);
    geom->setColorArray(colors, osg::Array::BIND_PER_VERTEX);
    geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINES, 0, resolution * 8));
    mGeometry = geom;
}

void CSVRender::BrushDraw::buildCircleGeometry(const float& radius, const osg::Vec3d& point)
{
    osg::ref_ptr<osg::Geometry> geom (new osg::Geometry());
    osg::ref_ptr<osg::Vec3Array> vertices (new osg::Vec3Array());
    osg::ref_ptr<osg::Vec4Array> colors (new osg::Vec4Array());
    const int amountOfPoints = (osg::PI * 2.0f) * radius / 20;
    const float step ((osg::PI * 2.0f) / static_cast<float>(amountOfPoints));
    const float brushOutlineHeight (1.0f);
    osg::Vec4f lineColor(1.0f, 1.0f, 1.0f, 0.6f);

    for (int i = 0; i < amountOfPoints + 2; i++)
    {
        float angle (i * step);
        vertices->push_back(osg::Vec3d(
            point.x() + radius * cosf(angle),
            point.y() + radius * sinf(angle),
            getIntersectionHeight(osg::Vec3d(
                point.x() + radius * cosf(angle),
                point.y() + radius * sinf(angle),
                point.z()) ) + brushOutlineHeight));
        colors->push_back(lineColor);
        angle = static_cast<float>(i + 1) * step;
        vertices->push_back(osg::Vec3d(
            point.x() + radius * cosf(angle),
            point.y() + radius * sinf(angle),
            getIntersectionHeight(osg::Vec3d(
                point.x() + radius * cosf(angle),
                point.y() + radius * sinf(angle),
                point.z()) ) + brushOutlineHeight));
        colors->push_back(lineColor);
    }

    geom->setVertexArray(vertices);
    geom->setColorArray(colors, osg::Array::BIND_PER_VERTEX);
    geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP, 0, amountOfPoints * 2));
    mGeometry = geom;
}

void CSVRender::BrushDraw::buildCustomGeometry(const float& radius, const osg::Vec3d& point)
{
    // Not implemented
}

void CSVRender::BrushDraw::update(osg::Vec3d point, int brushSize, CSVWidget::BrushShape toolShape)
{
    if (mBrushDrawNode->containsNode(mGeometry))
        mBrushDrawNode->removeChild(mGeometry);
    float radius = (mLandSizeFactor * brushSize) / 2;
    osg::Vec3d snapToGridPoint = point;
    if (mTextureMode)
    {
        std::pair<int, int> snapToGridXY = CSMWorld::CellCoordinates::toTextureCoords(point);
        float offsetToMiddle =  mLandSizeFactor * 0.5f;
        snapToGridPoint = osg::Vec3d(
            CSMWorld::CellCoordinates::textureGlobalXToWorldCoords(snapToGridXY.first) + offsetToMiddle,
            CSMWorld::CellCoordinates::textureGlobalYToWorldCoords(snapToGridXY.second) + offsetToMiddle,
            point.z());
    }
    else
    {
        std::pair<int, int> snapToGridXY = CSMWorld::CellCoordinates::toVertexCoords(point);
        snapToGridPoint = osg::Vec3d(
            CSMWorld::CellCoordinates::vertexGlobalToWorldCoords(snapToGridXY.first),
            CSMWorld::CellCoordinates::vertexGlobalToWorldCoords(snapToGridXY.second),
            point.z());
    }


    switch (toolShape)
    {
        case (CSVWidget::BrushShape_Point) :
            buildPointGeometry(snapToGridPoint);
            break;
        case (CSVWidget::BrushShape_Square) :
            buildSquareGeometry(radius, snapToGridPoint);
            break;
        case (CSVWidget::BrushShape_Circle) :
            buildCircleGeometry(radius, snapToGridPoint);
            break;
        case (CSVWidget::BrushShape_Custom) :
            buildSquareGeometry(1, snapToGridPoint);
            //buildCustomGeometry
            break;
    }

    mGeometry->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

    mBrushDrawNode->addChild(mGeometry);
}

void CSVRender::BrushDraw::hide()
{
    if (mBrushDrawNode->containsNode(mGeometry))
        mBrushDrawNode->removeChild(mGeometry);
}
