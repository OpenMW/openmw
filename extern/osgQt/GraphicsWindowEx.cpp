#include "GraphicsWindowEx"
#include "StateEx"

GraphicsWindowEx::GraphicsWindowEx(osg::GraphicsContext::Traits* traits)
{
    _traits = traits;

    init();
}

GraphicsWindowEx::GraphicsWindowEx(int x, int y, int width, int height)
{
    _traits = new osg::GraphicsContext::Traits();
    _traits->x = x;
    _traits->x = y;
    _traits->width = width;
    _traits->height = height;

    init();
}

void GraphicsWindowEx::init()
{
    if(valid())
    {
        // inject our "extended" state
        setState(new StateEx());
        getState()->setGraphicsContext(this);

        if(_traits.valid() && _traits->sharedContext.valid())
        {
            getState()->setContextID(_traits->sharedContext->getState()->getContextID());
            incrementContextIDUsageCount(getState()->getContextID());
        }
        else
        {
            getState()->setContextID(osg::GraphicsContext::createNewContextID());
        }
    }
}
