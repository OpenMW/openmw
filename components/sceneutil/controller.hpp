#ifndef OPENMW_COMPONENTS_SCENEUTIL_CONTROLLER_H
#define OPENMW_COMPONENTS_SCENEUTIL_CONTROLLER_H

#include <boost/shared_ptr.hpp>

#include <osg/NodeVisitor>

namespace SceneUtil
{

    class ControllerSource
    {
    public:
        virtual float getValue(osg::NodeVisitor* nv) = 0;
    };

    class FrameTimeSource : public ControllerSource
    {
    public:
        FrameTimeSource();
        virtual float getValue(osg::NodeVisitor* nv);
    };

    class ControllerFunction
    {
    public:
        virtual float calculate(float input) = 0;

        /// Get the "stop time" of the controller function, typically the maximum of the calculate() function.
        /// May not be meaningful for all types of controller functions.
        virtual float getMaximum() const = 0;
    };

    class Controller
    {
    public:
        Controller();

        bool hasInput() const;

        float getInputValue(osg::NodeVisitor* nv);

        boost::shared_ptr<ControllerSource> mSource;

        // The source value gets passed through this function before it's passed on to the DestValue.
        boost::shared_ptr<ControllerFunction> mFunction;
    };

    /// Pure virtual base class - visit() all controllers that are attached as UpdateCallbacks in a scene graph.
    class ControllerVisitor : public osg::NodeVisitor
    {
    public:
        ControllerVisitor();

        virtual void apply(osg::Node& node);
        virtual void apply(osg::Geode& geode);

        virtual void visit(osg::Node& node, Controller& ctrl) = 0;
    };

    class AssignControllerSourcesVisitor : public ControllerVisitor
    {
    public:
        AssignControllerSourcesVisitor();
        AssignControllerSourcesVisitor(boost::shared_ptr<ControllerSource> toAssign);

        /// Assign the wanted ControllerSource. May be overriden in derived classes.
        /// By default assigns the ControllerSource passed to the constructor of this class if no ControllerSource is assigned to that controller yet.
        virtual void visit(osg::Node& node, Controller& ctrl);

    private:
        boost::shared_ptr<ControllerSource> mToAssign;
    };

}

#endif
