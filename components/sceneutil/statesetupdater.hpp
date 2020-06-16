#ifndef OPENMW_COMPONENTS_SCENEUTIL_STATESETCONTROLLER_H
#define OPENMW_COMPONENTS_SCENEUTIL_STATESETCONTROLLER_H

#include <osg/NodeCallback>

namespace SceneUtil
{

    /// @brief Implements efficient per-frame updating of StateSets.
    /// @par With a naive update there would be race conditions when the OSG draw thread of the last frame
    ///     queues up a StateSet that we want to modify for the next frame. To solve this we could set the StateSet to
    ///     DYNAMIC data variance but that would undo all the benefits of the threading model - having the cull and draw
    ///     traversals run in parallel can yield up to 200% framerates.
    /// @par Race conditions are prevented using a "double buffering" scheme - we have two StateSets that take turns,
    ///     one StateSet we can write to, the second one is currently in use by the draw traversal of the last frame.
    /// @par Must be set as UpdateCallback or CullCallback on a Node. If set as a CullCallback, the StateSetUpdater operates on an empty StateSet, otherwise it operates on a clone of the node's existing StateSet.
    /// @note Do not add the same StateSetUpdater to multiple nodes.
    /// @note Do not add multiple StateSetControllers on the same Node as they will conflict - instead use the CompositeStateSetUpdater.
    class StateSetUpdater : public osg::NodeCallback
    {
    public:
        StateSetUpdater();
        StateSetUpdater(const StateSetUpdater& copy, const osg::CopyOp& copyop);

        META_Object(SceneUtil, StateSetUpdater)

        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv);

        /// Apply state - to override in derived classes
        /// @note Due to the double buffering approach you *have* to apply all state
        /// even if it has not changed since the last frame.
        virtual void apply(osg::StateSet* stateset, osg::NodeVisitor* nv) {}

        /// Set default state - optionally override in derived classes
        /// @par May be used e.g. to allocate StateAttributes.
        virtual void setDefaults(osg::StateSet* stateset) {}

    protected:
        /// Reset mStateSets, forcing a setDefaults() on the next frame. Can be used to change the defaults if needed.
        void reset();

    private:
        osg::ref_ptr<osg::StateSet> mStateSets[2];
    };

    /// @brief A variant of the StateSetController that can be made up of multiple controllers all controlling the same target.
    class CompositeStateSetUpdater : public StateSetUpdater
    {
    public:
        CompositeStateSetUpdater();
        CompositeStateSetUpdater(const CompositeStateSetUpdater& copy, const osg::CopyOp& copyop);

        META_Object(SceneUtil, CompositeStateSetUpdater)

        unsigned int getNumControllers();
        StateSetUpdater* getController(int i);

        void addController(StateSetUpdater* ctrl);

        virtual void apply(osg::StateSet* stateset, osg::NodeVisitor* nv);

    protected:

        virtual void setDefaults(osg::StateSet *stateset);

        std::vector<osg::ref_ptr<StateSetUpdater> > mCtrls;
    };

}

#endif
