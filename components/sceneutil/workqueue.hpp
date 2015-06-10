#ifndef OPENMW_COMPONENTS_SCENEUTIL_WORKQUEUE_H
#define OPENMW_COMPONENTS_SCENEUTIL_WORKQUEUE_H

#include <OpenThreads/Atomic>
#include <OpenThreads/Mutex>
#include <OpenThreads/Condition>
#include <OpenThreads/Thread>

#include <osg/Referenced>
#include <osg/ref_ptr>

#include <queue>

namespace SceneUtil
{

    class WorkTicket : public osg::Referenced
    {
    public:
        void waitTillDone();

        void signalDone();

    private:
        OpenThreads::Atomic mDone;
        OpenThreads::Mutex mMutex;
        OpenThreads::Condition mCondition;
    };

    class WorkItem
    {
    public:
        WorkItem();
        virtual ~WorkItem();

        /// Override in a derived WorkItem to perform actual work.
        /// By default, just signals the ticket that the work is done.
        virtual void doWork();

        osg::ref_ptr<WorkTicket> getTicket();

    protected:
        osg::ref_ptr<WorkTicket> mTicket;
    };

    class WorkQueue;

    class WorkThread : public OpenThreads::Thread
    {
    public:
        WorkThread(WorkQueue* workQueue);

        virtual void run();

    private:
        WorkQueue* mWorkQueue;
    };

    /// @brief A work queue that users can push work items onto, to be completed by one or more background threads.
    class WorkQueue
    {
    public:
        WorkQueue(int numWorkerThreads=1);
        ~WorkQueue();

        /// Add a new work item to the back of the queue.
        /// @par The returned WorkTicket may be used by the caller to wait until the work is complete.
        osg::ref_ptr<WorkTicket> addWorkItem(WorkItem* item);

        /// Get the next work item from the front of the queue. If the queue is empty, waits until a new item is added.
        /// If the workqueue is in the process of being destroyed, may return NULL.
        /// @note The caller must free the returned WorkItem
        WorkItem* removeWorkItem();

        void runThread();

    private:
        bool mIsReleased;
        std::queue<WorkItem*> mQueue;

        OpenThreads::Mutex mMutex;
        OpenThreads::Condition mCondition;

        std::vector<WorkThread*> mThreads;
    };



}

#endif
