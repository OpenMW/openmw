#include "glextensions.hpp"

namespace SceneUtil
{
    namespace
    {
        std::set<osg::observer_ptr<osg::GLExtensions>> sGLExtensions;

        class GLExtensionsObserver : public osg::Observer
        {
        public:
            static GLExtensionsObserver sInstance;

            void objectDeleted(void* referenced) override
            {
                sGLExtensions.erase(static_cast<osg::GLExtensions*>(referenced));
            }
        };

        GLExtensionsObserver GLExtensionsObserver::sInstance{};
    }

    osg::GLExtensions& getGLExtensions()
    {
        if (sGLExtensions.empty())
            throw std::runtime_error(
                "GetGLExtensionsOperation was not used when the current context was created or there is no current "
                "context");
        return **sGLExtensions.begin();
    }

    GetGLExtensionsOperation::GetGLExtensionsOperation()
        : GraphicsOperation("GetGLExtensionsOperation", false)
    {
    }

    void GetGLExtensionsOperation::operator()(osg::GraphicsContext* graphicsContext)
    {
        auto [itr, _] = sGLExtensions.emplace(graphicsContext->getState()->get<osg::GLExtensions>());
        (*itr)->addObserver(&GLExtensionsObserver::sInstance);
    }
}
