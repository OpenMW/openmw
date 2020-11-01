#ifndef OPENMW_COMPONENTS_UNREFQUEUE_H
#define OPENMW_COMPONENTS_UNREFQUEUE_H

#include <deque>

#include <osg/ref_ptr>
#include <osg/Referenced>

#include <components/sceneutil/workqueue.hpp>

namespace SceneUtil
{
    class WorkQueue;
    
    class UnrefWorkItem : public SceneUtil::WorkItem
    {
    public:
        std::deque<osg::ref_ptr<const osg::Referenced> > mObjects;
        void doWork() override;
    };

    /// @brief Handles unreferencing of objects through the WorkQueue. Typical use scenario
    /// would be the main thread pushing objects that are no longer needed, and the background thread deleting them.
    class UnrefQueue : public osg::Referenced
    {
    public:
        UnrefQueue();

        /// Adds an object to the list of objects to be unreferenced. Call from the main thread.
        void push(const osg::Referenced* obj);

        /// Adds a WorkItem to the given WorkQueue that will clear the list of objects in a worker thread, thus unreferencing them.
        /// Call from the main thread.
        void flush(SceneUtil::WorkQueue* workQueue);

        unsigned int getNumItems() const;

    private:
        osg::ref_ptr<UnrefWorkItem> mWorkItem;
    };

}

#endif
