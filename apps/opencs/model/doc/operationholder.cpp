#include "operationholder.hpp"

#include "operation.hpp"

CSMDoc::OperationHolder::OperationHolder(Operation* operation)
    : mOperation(nullptr)
    , mRunning(false)
{
    if (operation)
        setOperation(operation);
}

void CSMDoc::OperationHolder::setOperation(Operation* operation)
{
    mOperation = operation;
    mOperation->moveToThread(&mThread);

    connect(mOperation, &Operation::progress, this, &OperationHolder::progress);

    connect(mOperation, &Operation::reportMessage, this, &OperationHolder::reportMessage);

    connect(mOperation, &Operation::done, this, &OperationHolder::doneSlot);

    connect(this, &OperationHolder::abortSignal, mOperation, &Operation::abort);

    connect(&mThread, &QThread::started, mOperation, &Operation::run);
}

bool CSMDoc::OperationHolder::isRunning() const
{
    return mRunning;
}

void CSMDoc::OperationHolder::start()
{
    mRunning = true;
    mThread.start();
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
        mThread.quit();
        mThread.wait();
    }
}

void CSMDoc::OperationHolder::doneSlot(int type, bool failed)
{
    mRunning = false;
    mThread.quit();
    emit done(type, failed);
}
