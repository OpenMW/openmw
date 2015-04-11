#include "util.hpp"

namespace SceneUtil
{

void transformBoundingSphere (const osg::Matrix& matrix, osg::BoundingSphere& bsphere)
{
    osg::BoundingSphere::vec_type xdash = bsphere._center;
    xdash.x() += bsphere._radius;
    xdash = xdash*matrix;

    osg::BoundingSphere::vec_type ydash = bsphere._center;
    ydash.y() += bsphere._radius;
    ydash = ydash*matrix;

    osg::BoundingSphere::vec_type zdash = bsphere._center;
    zdash.z() += bsphere._radius;
    zdash = zdash*matrix;

    bsphere._center = bsphere._center*matrix;

    xdash -= bsphere._center;
    osg::BoundingSphere::value_type len_xdash = xdash.length();

    ydash -= bsphere._center;
    osg::BoundingSphere::value_type len_ydash = ydash.length();

    zdash -= bsphere._center;
    osg::BoundingSphere::value_type len_zdash = zdash.length();

    bsphere._radius = len_xdash;
    if (bsphere._radius<len_ydash) bsphere._radius = len_ydash;
    if (bsphere._radius<len_zdash) bsphere._radius = len_zdash;
}

osg::Vec4f colourFromRGB(unsigned int clr)
{
    osg::Vec4f colour(((clr >> 0) & 0xFF) / 255.0f,
                      ((clr >> 8) & 0xFF) / 255.0f,
                      ((clr >> 16) & 0xFF) / 255.0f, 1.f);
    return colour;
}

osg::Vec4f colourFromRGBA(unsigned int clr)
{
    osg::Vec4f colour(((clr >> 0) & 0xFF) / 255.0f,
                      ((clr >> 8) & 0xFF) / 255.0f,
                      ((clr >> 16) & 0xFF) / 255.0f,
                      ((clr >> 24) & 0xFF) / 255.0f);
    return colour;
}

}
