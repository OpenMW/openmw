#include "unrefqueue.hpp"

namespace SceneUtil
{
    namespace
    {
        struct ClearVector final : SceneUtil::WorkItem
        {
            std::vector<osg::ref_ptr<osg::Referenced>> mObjects;

            explicit ClearVector(std::vector<osg::ref_ptr<osg::Referenced>>&& objects)
                : mObjects(std::move(objects)) {}

            void doWork() override { mObjects.clear(); }
        };
    }

    void UnrefQueue::flush(SceneUtil::WorkQueue& workQueue)
    {
        if (mObjects.empty())
            return;

        // Move only objects to keep allocated storage in mObjects
        workQueue.addWorkItem(new ClearVector(std::vector<osg::ref_ptr<osg::Referenced>>(
            std::move_iterator(mObjects.begin()), std::move_iterator(mObjects.end()))));
        mObjects.clear();
    }
}
