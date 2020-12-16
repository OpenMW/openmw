#include "removedalphafunc.hpp"

#include <cassert>

#include <osg/State>

namespace Shader
{
    std::array<osg::ref_ptr<RemovedAlphaFunc>, GL_ALWAYS - GL_NEVER> RemovedAlphaFunc::sInstances{
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr
    };

    osg::ref_ptr<RemovedAlphaFunc> RemovedAlphaFunc::getInstance(GLenum func)
    {
        assert(func >= GL_NEVER && func <= GL_ALWAYS);
        if (!sInstances[func - GL_NEVER])
            sInstances[func - GL_NEVER] = new RemovedAlphaFunc(static_cast<osg::AlphaFunc::ComparisonFunction>(func), 1.0);
        return sInstances[func - GL_NEVER];
    }

    void RemovedAlphaFunc::apply(osg::State & state) const
    {
        // Hopefully, anything genuinely requiring the default alpha func of GL_ALWAYS explicitly sets it
        if (!state.getGlobalDefaultAttribute(ALPHAFUNC)->getType() != getType())
            state.setGlobalDefaultAttribute(static_cast<const osg::StateAttribute*>(cloneType()));
    }
}
