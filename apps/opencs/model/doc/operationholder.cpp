#include "operationholder.hpp"

#include "operation.hpp"

CSMDoc::OperationHolder::OperationHolder(Operation* operation)
    : mOperation(nullptr)
    , mRunning(false)
    , mUseThread(true)
{
    if (operation)
        setOperation(operation);
}

void CSMDoc::OperationHolder::setOperation(Operation* operation)
{
    mOperation = operation;

    if (mUseThread)
        mOperation->moveToThread(&mThread);

    connect(mOperation, &Operation::progress, this, &OperationHolder::progress);

    connect(mOperation, &Operation::reportMessage, this, &OperationHolder::reportMessage);

    connect(mOperation, &Operation::done, this, &OperationHolder::doneSlot);

    connect(this, &OperationHolder::abortSignal, mOperation, &Operation::abort);

    if (mUseThread)
        connect(&mThread, &QThread::started, mOperation, &Operation::run);
}

bool CSMDoc::OperationHolder::isRunning() const
{
    return mRunning;
}

void CSMDoc::OperationHolder::start()
{
    mRunning = true;

    if (mUseThread)
        mThread.start();
    else
        mOperation->run();
}

void CSMDoc::OperationHolder::abort()
{
    mRunning = false;
    emit abortSignal();
}

void CSMDoc::OperationHolder::abortAndWait()
{
    if (mRunning)
    {
        if (mUseThread)
        {
            abort();
            mThread.quit();
            mThread.wait();
        }
        else
        {
            abort();
        }
    }
}

void CSMDoc::OperationHolder::doneSlot(int type, bool failed)
{
    mRunning = false;

    if (mUseThread)
        mThread.quit();

    emit done(type, failed);
}

void CSMDoc::OperationHolder::setUseThread(bool useThread)
{
    mUseThread = useThread;
}
