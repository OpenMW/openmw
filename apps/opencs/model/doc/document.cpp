
#include "document.hpp"

#include <iostream>

void CSMDoc::Document::load (const std::vector<boost::filesystem::path>::const_iterator& begin,
    const std::vector<boost::filesystem::path>::const_iterator& end)
{
    for (std::vector<boost::filesystem::path>::const_iterator iter (begin); iter!=end; ++iter)
        std::cout << "pretending to load " << iter->string() << std::endl;

    /// \todo load content files
}

void CSMDoc::Document::createBase()
{
    static const char *sGlobals[] =
    {
        "Day", "DaysPassed", "GameHour", "Month", "PCRace", "PCVampire", "PCWerewolf", "PCYear", 0
    };

    for (int i=0; sGlobals[i]; ++i)
    {
        ESM::Global record;
        record.mId = sGlobals[i];
        record.mValue = i==0 ? 1 : 0;
        record.mType = ESM::VT_Float;
        getData().getGlobals().add (record);
    }
}

CSMDoc::Document::Document (const std::vector<boost::filesystem::path>& files, bool new_)
: mTools (mData)
{
    if (files.empty())
        throw std::runtime_error ("Empty content file sequence");

    /// \todo adjust last file name:
    /// \li make sure it is located in the data-local directory (adjust path if necessary)
    /// \li make sure the extension matches the new scheme (change it if necesarry)

    mName = files.back().filename().string();

    if (files.size()>1 || !new_)
    {
        std::vector<boost::filesystem::path>::const_iterator end = files.end();

        if (new_)
            --end;

        load (files.begin(), end);
    }

    if (new_ && files.size()==1)
        createBase();

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