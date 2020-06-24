#include "workqueue.hpp"

#include <components/debug/debuglog.hpp>

#include <numeric>

namespace SceneUtil
{

void WorkItem::waitTillDone()
{
    if (mDone)
        return;

    std::unique_lock<std::mutex> lock(mMutex);
    while (!mDone)
    {
        mCondition.wait(lock);
    }
}

void WorkItem::signalDone()
{
    {
        std::unique_lock<std::mutex> lock(mMutex);
        mDone = true;
    }
    mCondition.notify_all();
}

bool WorkItem::isDone() const
{
    return mDone;
}

WorkQueue::WorkQueue(int workerThreads)
    : mIsReleased(false)
{
    for (int i=0; i<workerThreads; ++i)
        mThreads.emplace_back(std::make_unique<WorkThread>(*this));
}

WorkQueue::~WorkQueue()
{
    {
        std::unique_lock<std::mutex> lock(mMutex);
        while (!mQueue.empty())
            mQueue.pop_back();
        mIsReleased = true;
        mCondition.notify_all();
    }

    mThreads.clear();
}

void WorkQueue::addWorkItem(osg::ref_ptr<WorkItem> item, bool front)
{
    if (item->isDone())
    {
        Log(Debug::Error) << "Error: trying to add a work item that is already completed";
        return;
    }

    std::unique_lock<std::mutex> lock(mMutex);
    if (front)
        mQueue.push_front(item);
    else
        mQueue.push_back(item);
    mCondition.notify_one();
}

osg::ref_ptr<WorkItem> WorkQueue::removeWorkItem()
{
    std::unique_lock<std::mutex> lock(mMutex);
    while (mQueue.empty() && !mIsReleased)
    {
        mCondition.wait(lock);
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
    std::unique_lock<std::mutex> lock(mMutex);
    return mQueue.size();
}

unsigned int WorkQueue::getNumActiveThreads() const
{
    return std::accumulate(mThreads.begin(), mThreads.end(), 0u,
        [] (auto r, const auto& t) { return r + t->isActive(); });
}

WorkThread::WorkThread(WorkQueue& workQueue)
    : mWorkQueue(&workQueue)
    , mActive(false)
    , mThread([this] { run(); })
{
}

WorkThread::~WorkThread()
{
    mThread.join();
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
