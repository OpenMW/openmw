#include "operationholder.hpp"

#include "../settings/usersettings.hpp"

#include "operation.hpp"

CSMDoc::OperationHolder::OperationHolder (Operation *operation) : mRunning (false)
{
    if (operation)
        setOperation (operation);
}

void CSMDoc::OperationHolder::setOperation (Operation *operation)
{
    mOperation = operation;
    mOperation->moveToThread (&mThread);

    connect (
        mOperation, SIGNAL (progress (int, int, int)),
        this, SIGNAL (progress (int, int, int)));

    connect (
        mOperation, SIGNAL (reportMessage (const CSMDoc::Message&, int)),
        this, SIGNAL (reportMessage (const CSMDoc::Message&, int)));

    connect (
        mOperation, SIGNAL (done (int, bool)),
        this, SLOT (doneSlot (int, bool)));

    connect (this, SIGNAL (abortSignal()), mOperation, SLOT (abort()));

    connect (&mThread, SIGNAL (started()), mOperation, SLOT (run()));

    connect (&CSMSettings::UserSettings::instance(), SIGNAL (userSettingUpdated (const QString&, const QStringList&)),
        mOperation, SLOT (updateUserSetting (const QString&, const QStringList&)));
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

void CSMDoc::OperationHolder::doneSlot (int type, bool failed)
{
    mRunning = false;
    mThread.quit();
    emit done (type, failed);
}
