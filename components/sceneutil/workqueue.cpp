#include "workqueue.hpp"

#include <components/debug/debuglog.hpp>

namespace SceneUtil
{

void WorkItem::waitTillDone()
{
    if (mDone > 0)
        return;

    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(mMutex);
    while (mDone == 0)
    {
        mCondition.wait(&mMutex);
    }
}

void WorkItem::signalDone()
{
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(mMutex);
        mDone.exchange(1);
    }
    mCondition.broadcast();
}

WorkItem::WorkItem()
{
}

WorkItem::~WorkItem()
{
}

bool WorkItem::isDone() const
{
    return (mDone > 0);
}

WorkQueue::WorkQueue(int workerThreads)
    : mIsReleased(false)
{
    for (int i=0; i<workerThreads; ++i)
    {
        WorkThread* thread = new WorkThread(this);
        mThreads.push_back(thread);
        thread->startThread();
    }
}

WorkQueue::~WorkQueue()
{
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(mMutex);
        while (!mQueue.empty())
            mQueue.pop_back();
        mIsReleased = true;
        mCondition.broadcast();
    }

    for (unsigned int i=0; i<mThreads.size(); ++i)
    {
        mThreads[i]->join();
        delete mThreads[i];
    }
}

void WorkQueue::addWorkItem(osg::ref_ptr<WorkItem> item, bool front)
{
    if (item->isDone())
    {
        Log(Debug::Error) << "Error: trying to add a work item that is already completed";
        return;
    }

    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(mMutex);
    if (front)
        mQueue.push_front(item);
    else
        mQueue.push_back(item);
    mCondition.signal();
}

osg::ref_ptr<WorkItem> WorkQueue::removeWorkItem()
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(mMutex);
    while (mQueue.empty() && !mIsReleased)
    {
        mCondition.wait(&mMutex);
    }
    if (!mQueue.empty())
    {
        osg::ref_ptr<WorkItem> item = mQueue.front();
        mQueue.pop_front();
        return item;
    }
    else
        return nullptr;
}

unsigned int WorkQueue::getNumItems() const
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(mMutex);
    return mQueue.size();
}

unsigned int WorkQueue::getNumActiveThreads() const
{
    unsigned int count = 0;
    for (unsigned int i=0; i<mThreads.size(); ++i)
    {
        if (mThreads[i]->isActive())
            ++count;
    }
    return count;
}

WorkThread::WorkThread(WorkQueue *workQueue)
    : mWorkQueue(workQueue)
    , mActive(false)
{
}

void WorkThread::run()
{
    while (true)
    {
        osg::ref_ptr<WorkItem> item = mWorkQueue->removeWorkItem();
        if (!item)
            return;
        mActive = true;
        item->doWork();
        item->signalDone();
        mActive = false;
    }
}

bool WorkThread::isActive() const
{
    return mActive;
}

}
