
#include "document.hpp"

CSMDoc::Document::Document (const std::string& name)
: mTools (mData)
{
    mName = name; ///< \todo replace with ESX list

    connect (&mUndoStack, SIGNAL (cleanChanged (bool)), this, SLOT (modificationStateChanged (bool)));

    connect (&mTools, SIGNAL (progress (int, int, int)), this, SLOT (progress (int, int, int)));
    connect (&mTools, SIGNAL (done (int)), this, SLOT (operationDone (int)));

     // dummy implementation -> remove when proper save is implemented.
    mSaveCount = 0;
    connect (&mSaveTimer, SIGNAL(timeout()), this, SLOT (saving()));
}

QUndoStack& CSMDoc::Document::getUndoStack()
{
    return mUndoStack;
}

int CSMDoc::Document::getState() const
{
    int state = 0;

    if (!mUndoStack.isClean())
        state |= State_Modified;

    if (mSaveCount)
        state |= State_Locked | State_Saving | State_Operation;

    if (int operations = mTools.getRunningOperations())
        state |= State_Locked | State_Operation | operations;

    return state;
}

const std::string& CSMDoc::Document::getName() const
{
    return mName;
}

void CSMDoc::Document::save()
{
    mSaveCount = 1;
    mSaveTimer.start (500);
    emit stateChanged (getState(), this);
    emit progress (1, 16, State_Saving, 1, this);
}

CSMWorld::UniversalId CSMDoc::Document::verify()
{
    CSMWorld::UniversalId id = mTools.runVerifier();
    emit stateChanged (getState(), this);
    return id;
}

void CSMDoc::Document::abortOperation (int type)
{
    mTools.abortOperation (type);

    if (type==State_Saving)
    {
        mSaveTimer.stop();
        emit stateChanged (getState(), this);
    }
}

void CSMDoc::Document::modificationStateChanged (bool clean)
{
    emit stateChanged (getState(), this);
}

void CSMDoc::Document::operationDone (int type)
{
    emit stateChanged (getState(), this);
}

void CSMDoc::Document::saving()
{
    ++mSaveCount;

    emit progress (mSaveCount, 16, State_Saving, 1, this);

    if (mSaveCount>15)
    {
            mSaveCount = 0;
            mSaveTimer.stop();
            mUndoStack.setClean();
            emit stateChanged (getState(), this);
    }
}

const CSMWorld::Data& CSMDoc::Document::getData() const
{
    return mData;
}

CSMWorld::Data& CSMDoc::Document::getData()
{
    return mData;
}

CSMTools::ReportModel *CSMDoc::Document::getReport (const CSMWorld::UniversalId& id)
{
    return mTools.getReport (id);
}

void CSMDoc::Document::progress (int current, int max, int type)
{
    emit progress (current, max, type, 1, this);
}