#include "controller.hpp"

#include "statesetupdater.hpp"

#include <osg/Drawable>
#include <osg/Geode>

namespace SceneUtil
{


    Controller::Controller()
    {
    }

    bool Controller::hasInput() const
    {
        return mSource.get() != NULL;
    }

    float Controller::getInputValue(osg::NodeVisitor* nv)
    {
        return mFunction->calculate(mSource->getValue(nv));
    }

    FrameTimeSource::FrameTimeSource()
    {
    }

    float FrameTimeSource::getValue(osg::NodeVisitor *nv)
    {
        return nv->getFrameStamp()->getSimulationTime();
    }

    AssignControllerSourcesVisitor::AssignControllerSourcesVisitor()
        : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
    {
    }

    AssignControllerSourcesVisitor::AssignControllerSourcesVisitor(boost::shared_ptr<ControllerSource> toAssign)
        : mToAssign(toAssign)
        , osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
    {
    }

    void AssignControllerSourcesVisitor::apply(osg::Node &node)
    {
        osg::NodeCallback* callback = node.getUpdateCallback();
        while (callback)
        {
            if (Controller* ctrl = dynamic_cast<Controller*>(callback))
                assign(node, *ctrl);
            if (CompositeStateSetUpdater* composite = dynamic_cast<CompositeStateSetUpdater*>(callback))
            {
                for (unsigned int i=0; i<composite->getNumControllers(); ++i)
                {
                    StateSetUpdater* statesetcontroller = composite->getController(i);
                    if (Controller* ctrl = dynamic_cast<Controller*>(statesetcontroller))
                        assign(node, *ctrl);
                }
            }

            callback = callback->getNestedCallback();
        }

        traverse(node);
    }

    void AssignControllerSourcesVisitor::apply(osg::Geode &geode)
    {
        for (unsigned int i=0; i<geode.getNumDrawables(); ++i)
        {
            osg::Drawable* drw = geode.getDrawable(i);
            osg::Drawable::UpdateCallback* callback = drw->getUpdateCallback();
            if (Controller* ctrl = dynamic_cast<Controller*>(callback))
                assign(geode, *ctrl);
        }
    }

    void AssignControllerSourcesVisitor::assign(osg::Node&, Controller &ctrl)
    {
        if (!ctrl.mSource.get())
            ctrl.mSource = mToAssign;
    }

}
