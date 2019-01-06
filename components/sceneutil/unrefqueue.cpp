#include "unrefqueue.hpp"

#include <cstdlib>
#include <iostream>

//#include <osg/Timer>

//#include <components/debug/debuglog.hpp>

namespace SceneUtil
{
    void UnrefWorkItem::doWork()
    {
        mObjects.clear();
    }

    UnrefQueue::UnrefQueue()
        : mUseWorkQueue(true)
    {
        mWorkItem = new UnrefWorkItem;
        if (getenv("OPENMW_DISABLE_DEFERRED_DELETE"))
        {
            std::cout << "Detected OPENMW_DISABLE_DEFERRED_DELETE, will disable threaded deletion of objects." << std::endl;
            mUseWorkQueue = false;
        }
    }

    void UnrefQueue::push(const osg::Referenced *obj)
    {
        mWorkItem->mObjects.push_back(obj);
    }

    void UnrefQueue::flush(SceneUtil::WorkQueue *workQueue)
    {
        if (mWorkItem->mObjects.empty())
            return;

        if (mUseWorkQueue)
            workQueue->addWorkItem(mWorkItem, true);

        mWorkItem = new UnrefWorkItem;
    }

    unsigned int UnrefQueue::getNumItems() const
    {
        return mWorkItem->mObjects.size();
    }

}
