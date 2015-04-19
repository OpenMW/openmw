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

    ControllerVisitor::ControllerVisitor()
        : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
    {

    }

    void ControllerVisitor::apply(osg::Node &node)
    {
        osg::NodeCallback* callback = node.getUpdateCallback();
        while (callback)
        {
            if (Controller* ctrl = dynamic_cast<Controller*>(callback))
                visit(node, *ctrl);
            if (CompositeStateSetUpdater* composite = dynamic_cast<CompositeStateSetUpdater*>(callback))
            {
                for (unsigned int i=0; i<composite->getNumControllers(); ++i)
                {
                    StateSetUpdater* statesetcontroller = composite->getController(i);
                    if (Controller* ctrl = dynamic_cast<Controller*>(statesetcontroller))
                        visit(node, *ctrl);
                }
            }

            callback = callback->getNestedCallback();
        }

        traverse(node);
    }

    void ControllerVisitor::apply(osg::Geode &geode)
    {
        for (unsigned int i=0; i<geode.getNumDrawables(); ++i)
        {
            osg::Drawable* drw = geode.getDrawable(i);
            osg::Drawable::UpdateCallback* callback = drw->getUpdateCallback();
            if (Controller* ctrl = dynamic_cast<Controller*>(callback))
                visit(geode, *ctrl);
        }
    }

    AssignControllerSourcesVisitor::AssignControllerSourcesVisitor()
        : ControllerVisitor()
    {
    }

    AssignControllerSourcesVisitor::AssignControllerSourcesVisitor(boost::shared_ptr<ControllerSource> toAssign)
        : ControllerVisitor()
        , mToAssign(toAssign)
    {
    }

    void AssignControllerSourcesVisitor::visit(osg::Node&, Controller &ctrl)
    {
        if (!ctrl.mSource.get())
            ctrl.mSource = mToAssign;
    }

    FindMaxControllerLengthVisitor::FindMaxControllerLengthVisitor()
        : SceneUtil::ControllerVisitor()
        , mMaxLength(0)
    {
    }

    void FindMaxControllerLengthVisitor::visit(osg::Node &, Controller &ctrl)
    {
        if (ctrl.mFunction)
            mMaxLength = std::max(mMaxLength, ctrl.mFunction->getMaximum());
    }

    float FindMaxControllerLengthVisitor::getMaxLength() const
    {
        return mMaxLength;
    }

}
