#ifndef OPENMW_COMPONENTS_SCENEUTIL_CONTROLLER_H
#define OPENMW_COMPONENTS_SCENEUTIL_CONTROLLER_H

#include <memory>
#include <osg/NodeVisitor>

namespace SceneUtil
{

    class ControllerSource
    {
    public:
        virtual ~ControllerSource() { }
        virtual float getValue(osg::NodeVisitor* nv) = 0;
    };

    class FrameTimeSource : public ControllerSource
    {
    public:
        FrameTimeSource();
        float getValue(osg::NodeVisitor* nv) override;
    };

    /// @note ControllerFunctions may be shared - you should not hold any state in it. That is why all its methods are declared const.
    class ControllerFunction
    {
    public:
        virtual ~ControllerFunction() = default;

        virtual float calculate(float input) const = 0;

        /// Get the "stop time" of the controller function, typically the maximum of the calculate() function.
        /// May not be meaningful for all types of controller functions.
        virtual float getMaximum() const = 0;
    };

    class Controller
    {
    public:
        Controller();
        virtual ~Controller() {}

        bool hasInput() const;

        float getInputValue(osg::NodeVisitor* nv);

        void setSource(std::shared_ptr<ControllerSource> source);
        void setFunction(std::shared_ptr<ControllerFunction> function);

        std::shared_ptr<ControllerSource> getSource() const;
        std::shared_ptr<ControllerFunction> getFunction() const;

    private:
        std::shared_ptr<ControllerSource> mSource;

        // The source value gets passed through this function before it's passed on to the DestValue.
        std::shared_ptr<ControllerFunction> mFunction;
    };

    /// Pure virtual base class - visit() all controllers that are attached as UpdateCallbacks in a scene graph.
    class ControllerVisitor : public osg::NodeVisitor
    {
    public:
        ControllerVisitor();

        void apply(osg::Node& node) override;

        // Technically not required as the default implementation would trickle down to apply(Node&) anyway,
        // but we'll shortcut instead to avoid the chain of virtual function calls
        void apply(osg::MatrixTransform& node) override;
        void apply(osg::Geometry& node) override;

        void applyNode(osg::Node& node);

        virtual void visit(osg::Node& node, Controller& ctrl) = 0;
    };

    class AssignControllerSourcesVisitor : public ControllerVisitor
    {
    public:
        AssignControllerSourcesVisitor();
        AssignControllerSourcesVisitor(std::shared_ptr<ControllerSource> toAssign);

        /// Assign the wanted ControllerSource. May be overridden in derived classes.
        /// By default assigns the ControllerSource passed to the constructor of this class if no ControllerSource is assigned to that controller yet.
        void visit(osg::Node& node, Controller& ctrl) override;

    private:
        std::shared_ptr<ControllerSource> mToAssign;
    };

    /// Finds the maximum of all controller functions in the given scene graph
    class FindMaxControllerLengthVisitor : public ControllerVisitor
    {
    public:
        FindMaxControllerLengthVisitor();

        void visit(osg::Node& , Controller& ctrl) override;

        float getMaxLength() const;

    private:
        float mMaxLength;
    };

}

#endif
