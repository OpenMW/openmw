#include "glextensions.hpp"

namespace SceneUtil
{
    namespace
    {
        osg::observer_ptr<osg::GLExtensions> sGLExtensions;
    }

    osg::GLExtensions& getGLExtensions()
    {
        if (!sGLExtensions)
            throw std::runtime_error(
                "GetGLExtensionsOperation was not used when the current context was created or there is no current "
                "context");
        return *sGLExtensions;
    }

    GetGLExtensionsOperation::GetGLExtensionsOperation()
        : GraphicsOperation("GetGLExtensionsOperation", false)
    {
    }

    void GetGLExtensionsOperation::operator()(osg::GraphicsContext* graphicsContext)
    {
        sGLExtensions = graphicsContext->getState()->get<osg::GLExtensions>();
    }
}
