#include "brushdraw.hpp"

#include <limits>

#include <osg/Group>
#include <osg/Geometry>
#include <osg/Array>
#include <osg/BlendFunc>
#include <osg/CullFace>
#include <osg/Material>

#include <osgUtil/LineSegmentIntersector>

#include "../widget/brushshapes.hpp"

#include "mask.hpp"

CSVRender::BrushDraw::BrushDraw(osg::ref_ptr<osg::Group> parentNode, bool textureMode) :
    mParentNode(parentNode)
{
    mBrushDrawNode = new osg::Group();
    mGeometry = new osg::Geometry();
    mBrushDrawNode->addChild(mGeometry);
    mParentNode->addChild(mBrushDrawNode);
    if (textureMode) mLandSizeFactor = ESM::Land::REAL_SIZE / ESM::Land::LAND_TEXTURE_SIZE / 2;
    else mLandSizeFactor = ESM::Land::REAL_SIZE / ESM::Land::LAND_SIZE / 2;
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
    end.z() = std::numeric_limits<int>::min();
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

void CSVRender::BrushDraw::buildPointGeometry(const float& radius, const osg::Vec3d& point)
{
    // Not implemented
}

void CSVRender::BrushDraw::buildSquareGeometry(const float& radius, const osg::Vec3d& point)
{
    osg::ref_ptr<osg::Geometry> geom = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices (new osg::Vec3Array());
    osg::ref_ptr<osg::Vec4Array> colors (new osg::Vec4Array());
    std::vector<osg::Vec3d> corners;
    const float brushOutLineHeight (200.0f);

    corners.push_back(osg::Vec3d(point.x() - radius, point.y() - radius, point.z()));
    corners.push_back(osg::Vec3d(point.x() + radius, point.y() - radius, point.z()));
    corners.push_back(osg::Vec3d(point.x() + radius, point.y() + radius, point.z()));
    corners.push_back(osg::Vec3d(point.x() - radius, point.y() + radius, point.z()));
    corners.push_back(osg::Vec3d(point.x() - radius, point.y() - radius, point.z()));

    for (const auto& point : corners)
    {
        vertices->push_back(osg::Vec3d(
            point.x(),
            point.y(),
            getIntersectionHeight(osg::Vec3d(
                point.x(),
                point.y(),
                point.z()) )));
        colors->push_back(osg::Vec4f(
            50.0f,
            50.0f,
            50.0f,
            100.0f));
        vertices->push_back(osg::Vec3d(
            point.x(),
            point.y(),
            getIntersectionHeight(osg::Vec3d(
                point.x(),
                point.y(),
                point.z())) + brushOutLineHeight));
        colors->push_back(osg::Vec4f(
            50.0f,
            50.0f,
            50.0f,
            100.0f));
    }

    geom->setVertexArray(vertices);
    geom->setColorArray(colors, osg::Array::BIND_PER_VERTEX);
    geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLE_STRIP, 0, (10 + 2) - 2));
    mGeometry = geom;
}

void CSVRender::BrushDraw::buildCircleGeometry(const float& radius, const osg::Vec3d& point)
{
    osg::ref_ptr<osg::Geometry> geom = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices (new osg::Vec3Array());
    osg::ref_ptr<osg::Vec4Array> colors (new osg::Vec4Array());
    const int amountOfPoints = (osg::PI * 2.0f) * radius / 20;
    const float step ((osg::PI * 2.0f) / static_cast<float>(amountOfPoints));
    const float brushOutLineHeight (200.0f);

    for (int i = 0; i < amountOfPoints + 2; i++)
    {
        float angle (static_cast<float>(i) * step);
        vertices->push_back(osg::Vec3d(
            point.x() + radius * cosf(angle),
            point.y() + radius * sinf(angle),
            getIntersectionHeight(osg::Vec3d(
                point.x() + radius * cosf(angle),
                point.y() + radius * sinf(angle),
                point.z()) )));
        colors->push_back(osg::Vec4f(
            50.0f,
            50.0f,
            50.0f,
            100.0f));
        angle = static_cast<float>(i + 1) * step;
        vertices->push_back(osg::Vec3d(
            point.x() + radius * cosf(angle),
            point.y() + radius * sinf(angle),
            getIntersectionHeight(osg::Vec3d(
                point.x() + radius * cosf(angle),
                point.y() + radius * sinf(angle),
                point.z()) ) + brushOutLineHeight));
        colors->push_back(osg::Vec4f(
            50.0f,
            50.0f,
            50.0f,
            100.0f));
    }

    geom->setVertexArray(vertices);
    geom->setColorArray(colors, osg::Array::BIND_PER_VERTEX);
    geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLE_STRIP, 0, (amountOfPoints + 2) * 2 - 2));
    mGeometry = geom;
}

void CSVRender::BrushDraw::buildCustomGeometry(const float& radius, const osg::Vec3d& point)
{
    // Not implemented
}

void CSVRender::BrushDraw::update(osg::Vec3d point, int brushSize, CSVWidget::BrushShape toolShape)
{
    if (mBrushDrawNode->containsNode(mGeometry)) mBrushDrawNode->removeChild(mGeometry);
    mBrushDrawNode->setNodeMask (Mask_EditModeCursor);
    float radius = static_cast<float>(brushSize * mLandSizeFactor);

    switch (toolShape)
    {
        case (CSVWidget::BrushShape_Point) :
            buildSquareGeometry(1, point);
            //buildPointGeometry(radius, point);
            break;
        case (CSVWidget::BrushShape_Square) :
            buildSquareGeometry(radius, point);
            break;
        case (CSVWidget::BrushShape_Circle) :
            buildCircleGeometry(radius, point);
            break;
        case (CSVWidget::BrushShape_Custom) :
            buildSquareGeometry(1, point);
            //buildCustomGeometry
            break;
    }

    osg::BlendFunc* blendFunc = new osg::BlendFunc(osg::BlendFunc::SRC_ALPHA, osg::BlendFunc::ONE_MINUS_SRC_ALPHA);
    mGeometry->getOrCreateStateSet()->setAttributeAndModes(blendFunc);

    mGeometry->getOrCreateStateSet()->setMode( GL_CULL_FACE, osg::StateAttribute::OFF );

    osg::ref_ptr<osg::Material> material = new osg::Material;
    material->setColorMode(osg::Material::AMBIENT);
    material->setAmbient (osg::Material::FRONT_AND_BACK, osg::Vec4(0.3, 0.3, 0.3, 1));
    material->setAlpha(osg::Material::FRONT_AND_BACK, 0.5);

    mGeometry->getOrCreateStateSet()->setAttributeAndModes(material.get(), osg::StateAttribute::ON);
    mGeometry->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
    mGeometry->getOrCreateStateSet()->setRenderingHint (osg::StateSet::TRANSPARENT_BIN);

    mBrushDrawNode->addChild(mGeometry);
}

void CSVRender::BrushDraw::hide()
{
    if (mBrushDrawNode->containsNode(mGeometry)) mBrushDrawNode->removeChild(mGeometry);
}
