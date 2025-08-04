#ifndef OPENMW_COMPONENTS_SCENEUTIL_DEPTH_H
#define OPENMW_COMPONENTS_SCENEUTIL_DEPTH_H

#include <osg/Depth>

#include "util.hpp"

#ifndef GL_DEPTH32F_STENCIL8_NV
#define GL_DEPTH32F_STENCIL8_NV 0x8DAC
#endif

#ifndef GL_DEPTH32F_STENCIL8
#define GL_DEPTH32F_STENCIL8 0x8CAD
#endif

#ifndef GL_FLOAT_32_UNSIGNED_INT_24_8_REV
#define GL_FLOAT_32_UNSIGNED_INT_24_8_REV 0x8DAD
#endif

#ifndef GL_DEPTH24_STENCIL8
#define GL_DEPTH24_STENCIL8 0x88F0
#endif

#ifndef GL_DEPTH_STENCIL_EXT
#define GL_DEPTH_STENCIL_EXT 0x84F9
#endif

#ifndef GL_UNSIGNED_INT_24_8_EXT
#define GL_UNSIGNED_INT_24_8_EXT 0x84FA
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
    osg::Matrix getReversedZProjectionMatrixAsOrtho(
        double left, double right, double bottom, double top, double near, double far);

    // Returns true if the GL format is a depth format
    bool isDepthFormat(GLenum format);

    // Returns true if the GL format is a depth+stencil format
    bool isDepthStencilFormat(GLenum format);

    // Returns the corresponding source format and type for the given internal format
    void getDepthFormatSourceFormatAndType(GLenum internalFormat, GLenum& sourceFormat, GLenum& sourceType);

    // Converts depth-stencil formats to their corresponding depth formats.
    GLenum getDepthFormatOfDepthStencilFormat(GLenum internalFormat);

    // Brief wrapper around an osg::Depth that applies the reversed depth function when a reversed depth buffer is in
    // use
    class AutoDepth : public osg::Depth
    {
    public:
        AutoDepth(
            // NB: OSG uses LESS test function by default, Morrowind uses LEQUAL
            osg::Depth::Function func = osg::Depth::LEQUAL, double zNear = 0.0, double zFar = 1.0,
            bool writeMask = true)
        {
            setFunction(func);
            setZNear(zNear);
            setZFar(zFar);
            setWriteMask(writeMask);
        }

        AutoDepth(const osg::Depth& copy, const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY)
            : osg::Depth(copy, copyop)
        {
        }

        osg::Object* cloneType() const override { return new AutoDepth; }
        osg::Object* clone(const osg::CopyOp& copyop) const override { return new AutoDepth(*this, copyop); }

        void apply(osg::State& state) const override
        {
            glDepthFunc(static_cast<GLenum>(AutoDepth::isReversed() ? getReversedDepthFunction() : getFunction()));
            glDepthMask(static_cast<GLboolean>(getWriteMask()));
#if defined(OSG_GLES1_AVAILABLE) || defined(OSG_GLES2_AVAILABLE) || defined(OSG_GLES3_AVAILABLE)
            glDepthRangef(getZNear(), getZFar());
#else
            glDepthRange(getZNear(), getZFar());
#endif
        }

        static void setReversed(bool reverseZ)
        {
            [[maybe_unused]] static const bool init = [&] {
                AutoDepth::sReversed = reverseZ;
                return true;
            }();
        }

        static bool isReversed()
        {
            return AutoDepth::sReversed;
        }

        static void setDepthFormat(GLenum format);

        static GLenum depthInternalFormat()
        {
            return AutoDepth::sDepthInternalFormat;
        }

        static GLenum depthSourceFormat()
        {
            return AutoDepth::sDepthSourceFormat;
        }

        static GLenum depthSourceType()
        {
            return AutoDepth::sDepthSourceType;
        }

    private:
        static inline bool sReversed = false;
        static inline GLenum sDepthSourceFormat = GL_DEPTH_COMPONENT;
        static inline GLenum sDepthInternalFormat = GL_DEPTH_COMPONENT24;
        static inline GLenum sDepthSourceType = GL_UNSIGNED_INT;

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
        ReplaceDepthVisitor()
            : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
        {
        }

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

    class SelectDepthFormatOperation : public osg::GraphicsOperation
    {
    public:
        SelectDepthFormatOperation()
            : GraphicsOperation("SelectDepthFormatOperation", false)
        {
        }

        void operator()(osg::GraphicsContext* graphicsContext) override;

        void setSupportedFormats(const std::vector<GLenum>& supportedFormats) { mSupportedFormats = supportedFormats; }

    private:
        std::vector<GLenum> mSupportedFormats;
    };
}

#endif
