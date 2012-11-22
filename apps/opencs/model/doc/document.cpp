
#include "document.hpp"

CSMDoc::Document::Document()
{
    connect (&mUndoStack, SIGNAL (cleanChanged (bool)), this, SLOT (modificationStateChanged (bool)));

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
        state |= State_Locked | State_Saving;

    return state;
}

void CSMDoc::Document::save()
{
    mSaveCount = 1;
    mSaveTimer.start (500);
}

void CSMDoc::Document::abortSave()
{
    mSaveTimer.stop();
}

void CSMDoc::Document::modificationStateChanged (bool clean)
{
    emit stateChanged (getState(), this);
}

void CSMDoc::Document::saving()
{
    ++mSaveCount;

    if (mSaveCount>15)
    {
            mSaveCount = 0;
            mSaveTimer.stop();
            mUndoStack.setClean();
    }
}