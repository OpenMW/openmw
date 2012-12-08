
#include "document.hpp"

CSMDoc::Document::Document (const std::string& name)
{
    mName = name; ///< \todo replace with ESX list

    connect (&mUndoStack, SIGNAL (cleanChanged (bool)), this, SLOT (modificationStateChanged (bool)));

    connect (&mTools, SIGNAL (progress (int, int, int)), this, SLOT (progress (int, int, int)));
    connect (&mTools, SIGNAL (done (int)), this, SLOT (operationDone (int)));

     // dummy implementation -> remove when proper save is implemented.
    mSaveCount = 0;
    connect (&mSaveTimer, SIGNAL(timeout()), this, SLOT (saving()));

     // dummy implementation -> remove when proper verify is implemented.
    mVerifyCount = 0;
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

    if (mVerifyCount)
        state |= State_Locked | State_Verifying | State_Operation;

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

void CSMDoc::Document::verify()
{
    mVerifyCount = 1;
    emit stateChanged (getState(), this);
    mTools.runVerifier();
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
    if (type==State_Verifying)
        mVerifyCount = 0;

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

void CSMDoc::Document::progress (int current, int max, int type)
{
    emit progress (current, max, type, 1, this);
}