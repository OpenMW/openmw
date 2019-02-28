#ifndef OPENMW_COMPONENTS_SCENEUTIL_DETOURDEBUGDRAW_H
#define OPENMW_COMPONENTS_SCENEUTIL_DETOURDEBUGDRAW_H

#include <DebugDraw.h>

#include <osg/Geometry>

namespace osg
{
    class Group;
}

namespace SceneUtil
{
    class DebugDraw : public duDebugDraw
    {
    public:
        DebugDraw(osg::Group& group, const osg::Vec3f& shift, float recastInvertedScaleFactor);

        void depthMask(bool state) override;

        void texture(bool state) override;

        void begin(osg::PrimitiveSet::Mode mode, float size);

        void begin(duDebugDrawPrimitives prim, float size) override;

        void vertex(const float* pos, unsigned int color) override;

        void vertex(const float x, const float y, const float z, unsigned int color) override;

        void vertex(const float* pos, unsigned int color, const float* uv) override;

        void vertex(const float x, const float y, const float z, unsigned int color,
                const float u, const float v) override;

        void end() override;

    private:
        osg::Group& mGroup;
        osg::Vec3f mShift;
        float mRecastInvertedScaleFactor;
        bool mDepthMask;
        osg::PrimitiveSet::Mode mMode;
        float mSize;
        osg::ref_ptr<osg::Vec3Array> mVertices;
        osg::ref_ptr<osg::Vec4Array> mColors;

        void addVertex(osg::Vec3f&& position);

        void addColor(osg::Vec4f&& value);
    };
}

#endif