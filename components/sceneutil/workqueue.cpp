#include "workqueue.hpp"

namespace SceneUtil
{

void WorkTicket::waitTillDone()
{
    if (mDone > 0)
        return;

    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(mMutex);
    while (mDone == 0)
    {
        mCondition.wait(&mMutex);
    }
}

void WorkTicket::signalDone()
{
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(mMutex);
        mDone.exchange(1);
    }
    mCondition.broadcast();
}

WorkItem::WorkItem()
    : mTicket(new WorkTicket)
{
    mTicket->setThreadSafeRefUnref(true);
}

WorkItem::~WorkItem()
{
}

void WorkItem::doWork()
{
    mTicket->signalDone();
}

osg::ref_ptr<WorkTicket> WorkItem::getTicket()
{
    return mTicket;
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
        while (mQueue.size())
        {
            WorkItem* item = mQueue.front();
            delete item;
            mQueue.pop();
        }
        mIsReleased = true;
        mCondition.broadcast();
    }

    for (unsigned int i=0; i<mThreads.size(); ++i)
    {
        mThreads[i]->join();
        delete mThreads[i];
    }
}

osg::ref_ptr<WorkTicket> WorkQueue::addWorkItem(WorkItem *item)
{
    osg::ref_ptr<WorkTicket> ticket = item->getTicket();
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(mMutex);
    mQueue.push(item);
    mCondition.signal();
    return ticket;
}

WorkItem *WorkQueue::removeWorkItem()
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(mMutex);
    while (!mQueue.size() && !mIsReleased)
    {
        mCondition.wait(&mMutex);
    }
    if (mQueue.size())
    {
        WorkItem* item = mQueue.front();
        mQueue.pop();
        return item;
    }
    else
        return NULL;
}

WorkThread::WorkThread(WorkQueue *workQueue)
    : mWorkQueue(workQueue)
{
}

void WorkThread::run()
{
    while (true)
    {
        WorkItem* item = mWorkQueue->removeWorkItem();
        if (!item)
            return;
        item->doWork();
        delete item;
    }
}

}
