#ifndef OPENMW_COMPONENTS_SCENEUTIL_DEPTH_H
#define OPENMW_COMPONENTS_SCENEUTIL_DEPTH_H

#include <osg/Depth>

#include "util.hpp"

#ifndef GL_DEPTH32F_STENCIL8_NV
#define GL_DEPTH32F_STENCIL8_NV 0x8DAC
#endif

namespace SceneUtil
{
    // Sets camera clear depth to 0 if reversed depth buffer is in use, 1 otherwise.
    void setCameraClearDepth(osg::Camera* camera);

    // Returns a perspective projection matrix for use with a reversed z-buffer
    // and an infinite far plane. This is derived by mapping the default z-range
    // of [0,1] to [1,0], then taking the limit as far plane approaches infinity.
    osg::Matrix getReversedZProjectionMatrixAsPerspectiveInf(double fov, double aspect, double near);

    // Returns a perspective projection matrix for use with a reversed z-buffer.
    osg::Matrix getReversedZProjectionMatrixAsPerspective(double fov, double aspect, double near, double far);

    // Returns an orthographic projection matrix for use with a reversed z-buffer.
    osg::Matrix getReversedZProjectionMatrixAsOrtho(double left, double right, double bottom, double top, double near, double far);

    // Returns true if the GL format is a floating point depth format.
    bool isFloatingPointDepthFormat(GLenum format);

    // Brief wrapper around an osg::Depth that applies the reversed depth function when a reversed depth buffer is in use
    class AutoDepth : public osg::Depth
    {
    public:
        AutoDepth(osg::Depth::Function func=osg::Depth::LESS, double zNear=0.0, double zFar=1.0, bool writeMask=true)
        {
            setFunction(func);
            setZNear(zNear);
            setZFar(zFar);
            setWriteMask(writeMask);
        }

        AutoDepth(const osg::Depth& copy, const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY) : osg::Depth(copy, copyop) {}

        osg::Object* cloneType() const override { return new AutoDepth; }
        osg::Object* clone(const osg::CopyOp& copyop) const override { return new AutoDepth(*this,copyop); }

        void apply(osg::State& state) const override
        {
            glDepthFunc(static_cast<GLenum>(AutoDepth::isReversed() ? getReversedDepthFunction() : getFunction()));
            glDepthMask(static_cast<GLboolean>(getWriteMask()));
        #if defined(OSG_GLES1_AVAILABLE) || defined(OSG_GLES2_AVAILABLE) || defined(OSG_GLES3_AVAILABLE)
            glDepthRangef(getZNear(),getZFar());
        #else
            glDepthRange(getZNear(),getZFar());
        #endif
        }

        static void setReversed(bool reverseZ)
        {
            static bool init = false;

            if (!init)
            {
                AutoDepth::sReversed = reverseZ;
                init = true;
            }
        }

        static bool isReversed()
        {
            return AutoDepth::sReversed;
        }

    private:

        static inline bool sReversed = false;

        osg::Depth::Function getReversedDepthFunction() const
        {
            const osg::Depth::Function func = getFunction();

            switch (func)
            {
            case osg::Depth::LESS:
                return osg::Depth::GREATER;
            case osg::Depth::LEQUAL:
                return osg::Depth::GEQUAL;
            case osg::Depth::GREATER:
                return osg::Depth::LESS;
            case osg::Depth::GEQUAL:
                return osg::Depth::LEQUAL;
            default:
                return func;
            }
        }

    };

    // Replaces all nodes osg::Depth state attributes with SceneUtil::AutoDepth.
    class ReplaceDepthVisitor : public osg::NodeVisitor
    {
    public:
        ReplaceDepthVisitor() : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN) {}

        void apply(osg::Node& node) override
        {
            osg::StateSet* stateSet = node.getStateSet();

            if (stateSet)
            {
                if (osg::Depth* depth = static_cast<osg::Depth*>(stateSet->getAttribute(osg::StateAttribute::DEPTH)))
                    stateSet->setAttribute(new SceneUtil::AutoDepth(*depth));
            };

            traverse(node);
        }
    };
}

#endif
