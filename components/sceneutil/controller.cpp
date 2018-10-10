#include "controller.hpp"

#include <algorithm>

#include "statesetupdater.hpp"

#include <osg/Drawable>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osg/NodeCallback>

namespace SceneUtil
{


    Controller::Controller()
    {
    }

    bool Controller::hasInput() const
    {
        return mSource.get() != nullptr;
    }

    float Controller::getInputValue(osg::NodeVisitor* nv)
    {
        if (mFunction)
            return mFunction->calculate(mSource->getValue(nv));
        else
            return mSource->getValue(nv);
    }

    void Controller::setSource(std::shared_ptr<ControllerSource> source)
    {
        mSource = source;
    }

    void Controller::setFunction(std::shared_ptr<ControllerFunction> function)
    {
        mFunction = function;
    }

    std::shared_ptr<ControllerSource> Controller::getSource() const
    {
        return mSource;
    }

    std::shared_ptr<ControllerFunction> Controller::getFunction() const
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
        applyNode(node);
    }

    void ControllerVisitor::apply(osg::MatrixTransform &node)
    {
        applyNode(node);
    }

    void ControllerVisitor::apply(osg::Geometry &node)
    {
        applyNode(node);
    }

    void ControllerVisitor::applyNode(osg::Node &node)
    {
        osg::Callback* callback = node.getUpdateCallback();
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

        if (node.getNumChildrenRequiringUpdateTraversal() > 0)
            traverse(node);
    }

    AssignControllerSourcesVisitor::AssignControllerSourcesVisitor()
        : ControllerVisitor()
    {
    }

    AssignControllerSourcesVisitor::AssignControllerSourcesVisitor(std::shared_ptr<ControllerSource> toAssign)
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
