#ifndef OPENMW_COMPONENTS_SCENEUTIL_STATESETCONTROLLER_H
#define OPENMW_COMPONENTS_SCENEUTIL_STATESETCONTROLLER_H

#include <osg/NodeCallback>

namespace SceneUtil
{

    /// @brief Implements efficient pre-frame updating of StateSets.
    /// @par With a naive update there would be race conditions when the OSG draw thread of the last frame
    ///     queues up a StateSet that we want to modify for the next frame. To solve this we could set the StateSet to
    ///     DYNAMIC data variance but that would undo all the benefits of the threading model - having the cull and draw
    ///     traversals run in parallel can yield up to 200% framerates.
    /// @par Race conditions are prevented using a "double buffering" scheme - we have two StateSets that take turns,
    ///     the first StateSet is the one we can write to, the second is the one currently in use by the draw traversal of the last frame.
    ///     After a frame is completed the places are swapped.
    /// @par Must be set as UpdateCallback on a Node.
    class StateSetController : public osg::NodeCallback
    {
    public:
        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv);

    protected:
        /// Apply state - to override in derived classes
        /// @note Due to the double buffering approach you *have* to apply all state
        /// even if it has not changed since the last frame.
        virtual void apply(osg::StateSet* stateset, osg::NodeVisitor* nv) = 0;

        /// Set default state - optionally override in derived classes
        /// @par May be used e.g. to allocate StateAttributes.
        virtual void setDefaults(osg::StateSet* stateset) {}

    private:
        osg::ref_ptr<osg::StateSet> mStateSets[2];
    };

}

#endif
