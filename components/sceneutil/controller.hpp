#ifndef OPENMW_COMPONENTS_SCENEUTIL_CONTROLLER_H
#define OPENMW_COMPONENTS_SCENEUTIL_CONTROLLER_H

#include <boost/shared_ptr.hpp>

#include <osg/NodeVisitor>

namespace SceneUtil
{

    class ControllerSource : public osg::Object
    {
    public:
        ControllerSource() {}
        ControllerSource(const ControllerSource& copy,
                                 const osg::CopyOp& copyop) : osg::Object(copy, copyop) {}
        virtual ~ControllerSource() {}

        META_Object(OpenMW, ControllerSource)

        virtual float getValue(osg::NodeVisitor* nv) { return 0.0f; }
    };

    class FrameTimeSource : public ControllerSource
    {
    public:
        FrameTimeSource() {}
        FrameTimeSource(const FrameTimeSource& copy,
                        const osg::CopyOp& copyop) : ControllerSource(copy, copyop) {}
        ~FrameTimeSource() {}

        META_Object(OpenMW, FrameTimeSource)

        virtual float getValue(osg::NodeVisitor* nv);
    };

    /// @note ControllerFunctions may be shared - you should not hold any state in it. That is why all its methods are declared const.
    class ControllerFunction : public osg::Object
    {
    public:
        ControllerFunction() { }
        ControllerFunction(const ControllerFunction& copy,
                           const osg::CopyOp& copyop) : osg::Object(copy, copyop) { }
        virtual ~ControllerFunction() { }

        META_Object(OpenMW, ControllerFunction)

        virtual float calculate(float input) const { return 0.0f; }

        /// Get the "stop time" of the controller function, typically the maximum of the calculate() function.
        /// May not be meaningful for all types of controller functions.
        virtual float getMaximum() const { return 0.0f; }
    };

    class Controller : virtual public osg::Object
    {
    public:
        Controller() {}
        Controller(const Controller& copy, const osg::CopyOp& copyop);
        virtual ~Controller() {}

        META_Object(OpenMW, Controller)

        bool hasInput() const;

        float getInputValue(osg::NodeVisitor* nv);

        ControllerSource* getSource() { return mSource.get(); }
        const ControllerSource* getSource() const { return mSource.get(); }
        void setSource(ControllerSource* source) { mSource = source; }
        ControllerFunction* getFunction() { return mFunction.get(); }
        const ControllerFunction* getFunction() const { return mFunction.get(); }
        void setFunction(ControllerFunction* function) { mFunction = function; }

    private:
        osg::ref_ptr<ControllerSource> mSource;

        // The source value gets passed through this function before it's passed on to the DestValue.
        osg::ref_ptr<ControllerFunction> mFunction;
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
        AssignControllerSourcesVisitor(osg::ref_ptr<ControllerSource> toAssign);

        /// Assign the wanted ControllerSource. May be overridden in derived classes.
        /// By default assigns the ControllerSource passed to the constructor of this class if no ControllerSource is assigned to that controller yet.
        virtual void visit(osg::Node& node, Controller& ctrl);

    private:
        osg::ref_ptr<ControllerSource> mToAssign;
    };

    /// Finds the maximum of all controller functions in the given scene graph
    class FindMaxControllerLengthVisitor : public ControllerVisitor
    {
    public:
        FindMaxControllerLengthVisitor();

        virtual void visit(osg::Node& , Controller& ctrl);

        float getMaxLength() const;

    private:
        float mMaxLength;
    };

}

#endif
