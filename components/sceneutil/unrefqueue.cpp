#include "unrefqueue.hpp"

#include <deque>

#include <osg/Object>
//#include <osg/Timer>
//#include <iostream>

#include <components/sceneutil/workqueue.hpp>

namespace SceneUtil
{

    class UnrefWorkItem : public SceneUtil::WorkItem
    {
    public:
        std::deque<osg::ref_ptr<const osg::Object> > mObjects;

        virtual void doWork()
        {
            //osg::Timer timer;
            //size_t objcount = mObjects.size();
            mObjects.clear();
            //std::cout << "cleared " << objcount << " objects in " << timer.time_m() << std::endl;
        }
    };

    UnrefQueue::UnrefQueue()
    {
        mWorkItem = new UnrefWorkItem;
    }

    void UnrefQueue::push(const osg::Object *obj)
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

}
