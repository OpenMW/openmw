#include "detourdebugdraw.hpp"

#include "depth.hpp"
#include "util.hpp"

#include <osg/BlendFunc>
#include <osg/Group>
#include <osg/LineWidth>
#include <osg/Material>
#include <osg/PolygonOffset>

#include <algorithm>

namespace
{
    osg::PrimitiveSet::Mode toOsgPrimitiveSetMode(duDebugDrawPrimitives value)
    {
        switch (value)
        {
            case DU_DRAW_POINTS:
                return osg::PrimitiveSet::POINTS;
            case DU_DRAW_LINES:
                return osg::PrimitiveSet::LINES;
            case DU_DRAW_TRIS:
                return osg::PrimitiveSet::TRIANGLES;
            case DU_DRAW_QUADS:
                return osg::PrimitiveSet::QUADS;
        }
        throw std::logic_error(
            "Can't convert duDebugDrawPrimitives to osg::PrimitiveSet::Mode, value=" + std::to_string(value));
    }

}

namespace SceneUtil
{
    DebugDraw::DebugDraw(osg::Group& group, const osg::ref_ptr<osg::StateSet>& stateSet, const osg::Vec3f& shift,
        float recastInvertedScaleFactor)
        : mGroup(group)
        , mStateSet(stateSet)
        , mShift(shift)
        , mRecastInvertedScaleFactor(recastInvertedScaleFactor)
        , mMode(osg::PrimitiveSet::POINTS)
        , mSize(1.0f)
    {
    }

    void DebugDraw::depthMask(bool) {}

    void DebugDraw::texture(bool) {}

    void DebugDraw::begin(osg::PrimitiveSet::Mode mode, float size)
    {
        mMode = mode;
        mVertices = new osg::Vec3Array;
        mColors = new osg::Vec4Array;
        mSize = size;
    }

    void DebugDraw::begin(duDebugDrawPrimitives prim, float size)
    {
        begin(toOsgPrimitiveSetMode(prim), size);
    }

    void DebugDraw::vertex(const float* pos, unsigned color)
    {
        vertex(pos[0], pos[1], pos[2], color);
    }

    void DebugDraw::vertex(const float x, const float y, const float z, unsigned color)
    {
        addVertex(osg::Vec3f(x, y, z));
        addColor(SceneUtil::colourFromRGBA(color));
    }

    void DebugDraw::vertex(const float* pos, unsigned color, const float* uv)
    {
        vertex(pos[0], pos[1], pos[2], color, uv[0], uv[1]);
    }

    void DebugDraw::vertex(const float x, const float y, const float z, unsigned color, const float, const float)
    {
        addVertex(osg::Vec3f(x, y, z));
        addColor(SceneUtil::colourFromRGBA(color));
    }

    void DebugDraw::end()
    {
        const osg::ref_ptr<osg::DrawArrays> drawArrays
            = new osg::DrawArrays(mMode, 0, static_cast<int>(mVertices->size()));

        osg::ref_ptr<osg::Geometry> geometry(new osg::Geometry);
        geometry->setStateSet(mStateSet);
        geometry->addPrimitiveSet(drawArrays);
        geometry->setVertexArray(std::exchange(mVertices, nullptr));
        geometry->setColorArray(std::exchange(mColors, nullptr), osg::Array::BIND_PER_VERTEX);

        mGroup.addChild(geometry);
    }

    void DebugDraw::addVertex(osg::Vec3f&& position)
    {
        std::swap(position.y(), position.z());
        mVertices->push_back(position * mRecastInvertedScaleFactor + mShift);
    }

    void DebugDraw::addColor(osg::Vec4f&& value)
    {
        mColors->push_back(value);
    }

    osg::ref_ptr<osg::StateSet> makeDetourGroupStateSet()
    {
        osg::ref_ptr<osg::Material> material = new osg::Material;
        material->setColorMode(osg::Material::AMBIENT_AND_DIFFUSE);

        const float polygonOffsetFactor = SceneUtil::AutoDepth::isReversed() ? 1.0f : -1.0f;
        const float polygonOffsetUnits = SceneUtil::AutoDepth::isReversed() ? 1.0f : -1.0f;
        osg::ref_ptr<osg::PolygonOffset> polygonOffset
            = new osg::PolygonOffset(polygonOffsetFactor, polygonOffsetUnits);

        osg::ref_ptr<osg::StateSet> stateSet = new osg::StateSet;
        stateSet->setAttribute(material);
        stateSet->setAttributeAndModes(polygonOffset);
        return stateSet;
    }
}
