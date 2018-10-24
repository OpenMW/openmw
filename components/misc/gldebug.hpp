#ifndef OPENMW_COMPONENTS_MISC_GLDEBUG_H
#define OPENMW_COMPONENTS_MISC_GLDEBUG_H

#include <osgViewer/ViewerEventHandlers>

class EnableGLDebugOperation : public osg::GraphicsOperation
{
public:
    EnableGLDebugOperation();

    virtual void operator()(osg::GraphicsContext* graphicsContext);

private:
    OpenThreads::Mutex mMutex;
};
#endif
