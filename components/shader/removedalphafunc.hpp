#ifndef OPENMW_COMPONENTS_REMOVEDALPHAFUNC_H
#define OPENMW_COMPONENTS_REMOVEDALPHAFUNC_H

#include <array>

#include <osg/AlphaFunc>

namespace Shader
{
    // State attribute used when shader visitor replaces the deprecated alpha function with a shader
    // Prevents redundant glAlphaFunc calls and lets the shadowsbin know the stateset had alpha testing
    class RemovedAlphaFunc : public osg::AlphaFunc
    {
    public:
        // Get a singleton-like instance with the right func (but a default threshold)
        static osg::ref_ptr<RemovedAlphaFunc> getInstance(GLenum func);

        RemovedAlphaFunc()
            : osg::AlphaFunc()
        {}

        RemovedAlphaFunc(ComparisonFunction func, float ref)
            : osg::AlphaFunc(func, ref)
        {}

        RemovedAlphaFunc(const RemovedAlphaFunc& raf, const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY)
            : osg::AlphaFunc(raf, copyop)
        {}

        META_StateAttribute(Shader, RemovedAlphaFunc, ALPHAFUNC);

        void apply(osg::State& state) const override {}

    protected:
        virtual ~RemovedAlphaFunc() = default;

        static std::array<osg::ref_ptr<RemovedAlphaFunc>, GL_ALWAYS - GL_NEVER + 1> sInstances;
    };
}
#endif //OPENMW_COMPONENTS_REMOVEDALPHAFUNC_H
