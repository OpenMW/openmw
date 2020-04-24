#include "detourdebugdraw.hpp"
#include "util.hpp"

#include <components/detournavigator/debug.hpp>

#include <osg/BlendFunc>
#include <osg/Group>
#include <osg/LineWidth>

#define OPENMW_TO_STRING(X) #X
#define OPENMW_LINE_STRING OPENMW_TO_STRING(__LINE__)

namespace
{
    using DetourNavigator::operator<<;

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
        throw std::logic_error("Can't convert duDebugDrawPrimitives to osg::PrimitiveSet::Mode, value="
                               + std::to_string(value));
    }

}

namespace SceneUtil
{
    DebugDraw::DebugDraw(osg::Group& group, const osg::Vec3f& shift, float recastInvertedScaleFactor)
        : mGroup(group)
        , mShift(shift)
        , mRecastInvertedScaleFactor(recastInvertedScaleFactor)
        , mDepthMask(false)
        , mMode(osg::PrimitiveSet::POINTS)
        , mSize(1.0f)
    {
    }

    void DebugDraw::depthMask(bool state)
    {
        mDepthMask = state;
    }

    void DebugDraw::texture(bool)
    {
    }

    void DebugDraw::begin(osg::PrimitiveSet::Mode mode, float size)
    {
        mMode = mode;
        mVertices = new osg::Vec3Array;
        mColors = new osg::Vec4Array;
        mSize = size * mRecastInvertedScaleFactor;
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
        osg::ref_ptr<osg::StateSet> stateSet(new osg::StateSet);
        stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
        stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
        stateSet->setMode(GL_DEPTH, (mDepthMask ? osg::StateAttribute::ON : osg::StateAttribute::OFF));
        stateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
        stateSet->setAttributeAndModes(new osg::LineWidth(mSize));
        stateSet->setAttributeAndModes(new osg::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

        osg::ref_ptr<osg::Geometry> geometry(new osg::Geometry);
        geometry->setStateSet(stateSet);
        geometry->setVertexArray(mVertices);
        geometry->setColorArray(mColors, osg::Array::BIND_PER_VERTEX);
        geometry->addPrimitiveSet(new osg::DrawArrays(mMode, 0, static_cast<int>(mVertices->size())));

        mGroup.addChild(geometry);
        mColors.release();
        mVertices.release();
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
}

#undef OPENMW_TO_STRING
#undef OPENMW_LINE_STRING
