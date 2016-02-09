#ifndef OPENMW_COMPONENTS_UNREFQUEUE_H
#define OPENMW_COMPONENTS_UNREFQUEUE_H

#include <osg/ref_ptr>
#include <osg/Referenced>

namespace osg
{
    class Object;
}

namespace SceneUtil
{
    class WorkQueue;
    class UnrefWorkItem;

    /// @brief Handles unreferencing of objects through the WorkQueue. Typical use scenario
    /// would be the main thread pushing objects that are no longer needed, and the background thread deleting them.
    class UnrefQueue : public osg::Referenced
    {
    public:
        UnrefQueue();

        /// Adds an object to the list of objects to be unreferenced. Call from the main thread.
        void push(const osg::Object* obj);

        /// Adds a WorkItem to the given WorkQueue that will clear the list of objects in a worker thread, thus unreferencing them.
        /// Call from the main thread.
        void flush(SceneUtil::WorkQueue* workQueue);

    private:
        osg::ref_ptr<UnrefWorkItem> mWorkItem;
    };

}

#endif
