#include "brushdraw.hpp"

#include <osg/Group>
#include <osg/Geometry>
#include <osg/Array>
#include <osg/BlendFunc>
#include <osg/CullFace>
#include <osg/Material>

#include <osgUtil/LineSegmentIntersector>

#include "mask.hpp"

CSVRender::BrushDraw::BrushDraw(osg::Group* parentNode) :
    mParentNode(parentNode)
{
    mBrushDrawNode = new osg::Group();
    mGeometry = new osg::Geometry();
    mBrushDrawNode->addChild(mGeometry);
    mParentNode->addChild(mBrushDrawNode);
}

CSVRender::BrushDraw::~BrushDraw()
{
    if (mBrushDrawNode->containsNode(mGeometry)) mBrushDrawNode->removeChild(mGeometry);
    if (mParentNode->containsNode(mBrushDrawNode)) mParentNode->removeChild(mBrushDrawNode);
}

float CSVRender::BrushDraw::getIntersectionHeight (const osg::Vec3d& point)
{
    osg::Vec3d start = point;
    osg::Vec3d end = point;
    start.z() += 8000.0f; // these numbers need fixing
    end.z() -= 8000.0f;
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


void CSVRender::BrushDraw::buildGeometry(const float& radius, const osg::Vec3d& point, int amountOfPoints)
{
    osg::ref_ptr<osg::Geometry> geom = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices (new osg::Vec3Array());
    osg::ref_ptr<osg::Vec4Array> colors (new osg::Vec4Array());
    const float step ((osg::PI * 2.0f) / static_cast<float>(amountOfPoints));

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
                point.z()) ) + 200.0f));
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

void CSVRender::BrushDraw::update(osg::Vec3d point, int brushSize)
{
    if (mBrushDrawNode->containsNode(mGeometry)) mBrushDrawNode->removeChild(mGeometry);
    mBrushDrawNode->setNodeMask (Mask_EditModeCursor);
    float radius = static_cast<float>(brushSize * mLandSizeFactor);
    int amountOfPoints = (osg::PI * 2.0f) * radius / 20;

    buildGeometry(radius, point, amountOfPoints);

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
