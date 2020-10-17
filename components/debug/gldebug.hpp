#ifndef OPENMW_COMPONENTS_DEBUG_GLDEBUG_H
#define OPENMW_COMPONENTS_DEBUG_GLDEBUG_H

#include <osgViewer/ViewerEventHandlers>

namespace Debug
{
    class EnableGLDebugOperation : public osg::GraphicsOperation
    {
    public:
        EnableGLDebugOperation();

        void operator()(osg::GraphicsContext* graphicsContext) override;

    private:
        OpenThreads::Mutex mMutex;
    };

    bool shouldDebugOpenGL();
}
#endif
