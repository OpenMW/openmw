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

    class AssignControllerSourcesVisitor : public osg::NodeVisitor
    {
    public:
        AssignControllerSourcesVisitor();
        AssignControllerSourcesVisitor(boost::shared_ptr<ControllerSource> toAssign);

        virtual void apply(osg::Node& node);
        virtual void apply(osg::Geode& geode);

        /// Assign the wanted ControllerSource. May be overriden in derived classes.
        /// By default assigns the ControllerSource passed to the constructor of this class if no ControllerSource is assigned to that controller yet.
        virtual void assign(osg::Node& node, Controller& ctrl);

    private:
        boost::shared_ptr<ControllerSource> mToAssign;
    };

}

#endif
