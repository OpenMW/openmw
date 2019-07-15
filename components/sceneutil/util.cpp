#include "util.hpp"
#include <osg/Node>
#include <components/esm/loadcell.hpp>
#include <components/esm/loadland.hpp>

namespace SceneUtil
{

const float cellSize = static_cast<float>(ESM::Land::REAL_SIZE);

osg::Vec3 getCellOrigin(const ESM::Cell *c)
{
    const ESM::CellId::CellIndex &cellid =c->getCellId().mIndex;
    return osg::Vec3( (static_cast<float>(cellid.mX)) * cellSize,
                         (static_cast<float>(cellid.mY)) * cellSize, 0);
}

void transformBoundingSphere (const osg::Matrixf& matrix, osg::BoundingSphere& bsphere)
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
    osg::BoundingSphere::value_type sqrlen_xdash = xdash.length2();

    ydash -= bsphere._center;
    osg::BoundingSphere::value_type sqrlen_ydash = ydash.length2();

    zdash -= bsphere._center;
    osg::BoundingSphere::value_type sqrlen_zdash = zdash.length2();

    bsphere._radius = sqrlen_xdash;
    if (bsphere._radius<sqrlen_ydash) bsphere._radius = sqrlen_ydash;
    if (bsphere._radius<sqrlen_zdash) bsphere._radius = sqrlen_zdash;
    bsphere._radius = sqrtf(bsphere._radius);
}

osg::Vec4f colourFromRGB(unsigned int clr)
{
    osg::Vec4f colour(((clr >> 0) & 0xFF) / 255.0f,
                      ((clr >> 8) & 0xFF) / 255.0f,
                      ((clr >> 16) & 0xFF) / 255.0f, 1.f);
    return colour;
}

osg::Vec4f colourFromRGBA(unsigned int value)
{
    return osg::Vec4f(makeOsgColorComponent(value, 0), makeOsgColorComponent(value, 8),
                      makeOsgColorComponent(value, 16), makeOsgColorComponent(value, 24));
}

float makeOsgColorComponent(unsigned int value, unsigned int shift)
{
    return float((value >> shift) & 0xFFu) / 255.0f;
}

bool hasUserDescription(const osg::Node* node, const std::string pattern)
{
    if (node == nullptr)
        return false;

    const osg::UserDataContainer* udc = node->getUserDataContainer();
    if (udc && udc->getNumDescriptions() > 0)
    {
        for (auto& descr : udc->getDescriptions())
        {
            if (descr == pattern)
                return true;
        }
    }

    return false;
}


}
