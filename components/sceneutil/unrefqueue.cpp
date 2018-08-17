#include "unrefqueue.hpp"

#include <deque>

//#include <osg/Timer>

//#include <components/debug/debuglog.hpp>
#include <components/sceneutil/workqueue.hpp>

namespace SceneUtil
{

    class UnrefWorkItem : public SceneUtil::WorkItem
    {
    public:
        std::deque<osg::ref_ptr<const osg::Referenced> > mObjects;

        virtual void doWork()
        {
            //osg::Timer timer;
            //size_t objcount = mObjects.size();
            mObjects.clear();
            //Log(Debug::Verbose) << "cleared " << objcount << " objects in " << timer.time_m();
        }
    };

    UnrefQueue::UnrefQueue()
    {
        mWorkItem = new UnrefWorkItem;
    }

    void UnrefQueue::push(const osg::Referenced *obj)
    {
        mWorkItem->mObjects.push_back(obj);
    }

    void UnrefQueue::flush(SceneUtil::WorkQueue *workQueue)
    {
        if (mWorkItem->mObjects.empty())
            return;

        workQueue->addWorkItem(mWorkItem, true);

        mWorkItem = new UnrefWorkItem;
    }

    unsigned int UnrefQueue::getNumItems() const
    {
        return mWorkItem->mObjects.size();
    }

}
