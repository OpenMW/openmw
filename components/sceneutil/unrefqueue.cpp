#include "unrefqueue.hpp"

#include <components/sceneutil/workqueue.hpp>

namespace SceneUtil
{
    namespace
    {
        struct ClearVector final : SceneUtil::WorkItem
        {
            std::vector<std::shared_ptr<void>> mObjects;

            explicit ClearVector(std::vector<std::shared_ptr<void>>&& objects)
                : mObjects(std::move(objects))
            {
            }

            void doWork() override { mObjects.clear(); }
        };
    }

    void UnrefQueue::flush(SceneUtil::WorkQueue& workQueue)
    {
        if (mObjects.empty())
            return;

        // Move only objects to keep allocated storage in mObjects
        workQueue.addWorkItem(std::make_shared<ClearVector>(std::vector<std::shared_ptr<void>>(
            std::move_iterator(mObjects.begin()), std::move_iterator(mObjects.end()))));
        mObjects.clear();
    }
}
