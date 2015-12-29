#include "controller.hpp"

#include "statesetupdater.hpp"

#include <osg/Drawable>
#include <osg/Geode>
#include <osg/NodeCallback>
#include <osg/Version>

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
        if (mFunction)
            return mFunction->calculate(mSource->getValue(nv));
        else
            return mSource->getValue(nv);
    }

    void Controller::setSource(boost::shared_ptr<ControllerSource> source)
    {
        mSource = source;
    }

    void Controller::setFunction(boost::shared_ptr<ControllerFunction> function)
    {
        mFunction = function;
    }

    boost::shared_ptr<ControllerSource> Controller::getSource() const
    {
        return mSource;
    }

    boost::shared_ptr<ControllerFunction> Controller::getFunction() const
    {
        return mFunction;
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
#if OSG_VERSION_GREATER_OR_EQUAL(3,3,3)
        osg::Callback* callback = node.getUpdateCallback();
#else
        osg::NodeCallback* callback = node.getUpdateCallback();
#endif
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

#if OSG_VERSION_GREATER_OR_EQUAL(3,3,3)
            osg::Callback* callback = drw->getUpdateCallback();
#else
            osg::Drawable::UpdateCallback* callback = drw->getUpdateCallback();
#endif

            if (Controller* ctrl = dynamic_cast<Controller*>(callback))
                visit(geode, *ctrl);
        }

        apply(static_cast<osg::Node&>(geode));
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
        if (!ctrl.getSource())
            ctrl.setSource(mToAssign);
    }

    FindMaxControllerLengthVisitor::FindMaxControllerLengthVisitor()
        : SceneUtil::ControllerVisitor()
        , mMaxLength(0)
    {
    }

    void FindMaxControllerLengthVisitor::visit(osg::Node &, Controller &ctrl)
    {
        if (ctrl.getFunction())
            mMaxLength = std::max(mMaxLength, ctrl.getFunction()->getMaximum());
    }

    float FindMaxControllerLengthVisitor::getMaxLength() const
    {
        return mMaxLength;
    }

}
