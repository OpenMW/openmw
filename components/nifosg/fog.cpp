#include "fog.hpp"

#include <osg/Matrix>
#include <osg/State>

namespace NifOsg
{

    Fog::Fog()
        : osg::Fog()
    {
    }

    Fog::Fog(const Fog& copy, const osg::CopyOp& copyop)
        : osg::Fog(copy, copyop)
        , mDepth(copy.mDepth)
    {
    }

    void Fog::apply(osg::State& state) const
    {
        osg::Fog::apply(state);
#ifdef OSG_GL_FIXED_FUNCTION_AVAILABLE
        float fov, aspect, near, far;
        state.getProjectionMatrix().getPerspective(fov, aspect, near, far);
        glFogf(GL_FOG_START, near * mDepth + far * (1.f - mDepth));
        glFogf(GL_FOG_END, far);
#endif
    }

}
