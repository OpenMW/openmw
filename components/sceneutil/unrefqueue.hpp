#ifndef OPENMW_COMPONENTS_UNREFQUEUE_H
#define OPENMW_COMPONENTS_UNREFQUEUE_H

#include "workqueue.hpp"

#include <osg/ref_ptr>
#include <osg/Referenced>

#include <vector>

namespace SceneUtil
{
    class WorkQueue;

    /// @brief Handles unreferencing of objects through the WorkQueue. Typical use scenario
    /// would be the main thread pushing objects that are no longer needed, and the background thread deleting them.
    class UnrefQueue
    {
    public:
        /// Adds an object to the list of objects to be unreferenced. Call from the main thread.
        void push(osg::ref_ptr<osg::Referenced>&& obj) { mObjects.push_back(std::move(obj)); }

        void push(const osg::ref_ptr<osg::Referenced>& obj) { mObjects.push_back(obj); }

        /// Adds a WorkItem to the given WorkQueue that will clear the list of objects in a worker thread,
        /// thus unreferencing them. Call from the main thread.
        void flush(SceneUtil::WorkQueue& workQueue);

        std::size_t getSize() const { return mObjects.size(); }

    private:
        std::vector<osg::ref_ptr<osg::Referenced>> mObjects;
    };
}

#endif
