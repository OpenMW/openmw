#include "loader.hpp"

#include <iostream>

#include "../tools/reportmodel.hpp"

#include "document.hpp"

CSMDoc::Loader::Stage::Stage() : mFile (0), mRecordsLoaded (0), mRecordsLeft (false) {}


CSMDoc::Loader::Loader()
    : mShouldStop(false)
{
    mTimer = new QTimer (this);

    connect (mTimer, SIGNAL (timeout()), this, SLOT (load()));
    mTimer->start();
}

QWaitCondition& CSMDoc::Loader::hasThingsToDo()
{
    return mThingsToDo;
}

void CSMDoc::Loader::stop()
{
    mShouldStop = true;
}

void CSMDoc::Loader::load()
{
    if (mDocuments.empty())
    {
        mMutex.lock();
        mThingsToDo.wait (&mMutex);
        mMutex.unlock();

        if (mShouldStop)
            mTimer->stop();

        return;
    }

    std::vector<std::pair<Document *, Stage> >::iterator iter = mDocuments.begin();

    Document *document = iter->first;

    int size = static_cast<int> (document->getContentFiles().size());
    int editedIndex = size-1; // index of the file to be edited/created

    if (document->isNew())
        --size;

    bool done = false;

    try
    {
        if (iter->second.mRecordsLeft)
        {
            Messages messages (Message::Severity_Error);
            const int batchingSize = 50;
            for (int i=0; i<batchingSize; ++i) // do not flood the system with update signals
                if (document->getData().continueLoading (messages))
                {
                    iter->second.mRecordsLeft = false;
                    break;
                }
                else
                    ++(iter->second.mRecordsLoaded);

            CSMWorld::UniversalId log (CSMWorld::UniversalId::Type_LoadErrorLog, 0);

            { // silence a g++ warning
            for (CSMDoc::Messages::Iterator messageIter (messages.begin());
                messageIter!=messages.end(); ++messageIter)
            {
                document->getReport (log)->add (*messageIter);
                emit loadMessage (document, messageIter->mMessage);
            }
            }

            emit nextRecord (document, iter->second.mRecordsLoaded);

            return;
        }

        if (iter->second.mFile<size)
        {
            boost::filesystem::path path = document->getContentFiles()[iter->second.mFile];

            int steps = document->getData().startLoading (path, iter->second.mFile!=editedIndex, false);
            iter->second.mRecordsLeft = true;
            iter->second.mRecordsLoaded = 0;

            emit nextStage (document, path.filename().string(), steps);
        }
        else if (iter->second.mFile==size)
        {
            int steps = document->getData().startLoading (document->getProjectPath(), false, true);
            iter->second.mRecordsLeft = true;
            iter->second.mRecordsLoaded = 0;

            emit nextStage (document, "Project File", steps);
        }
        else
        {
            done = true;
        }

        ++(iter->second.mFile);
    }
    catch (const std::exception& e)
    {
        mDocuments.erase (iter);
        emit documentNotLoaded (document, e.what());
        return;
    }

    if (done)
    {
        mDocuments.erase (iter);
        emit documentLoaded (document);
    }
}

void CSMDoc::Loader::loadDocument (CSMDoc::Document *document)
{
    mDocuments.emplace_back (document, Stage());
}

void CSMDoc::Loader::abortLoading (CSMDoc::Document *document)
{
    for (std::vector<std::pair<Document *, Stage> >::iterator iter = mDocuments.begin();
        iter!=mDocuments.end(); ++iter)
    {
        if (iter->first==document)
        {
            mDocuments.erase (iter);
            emit documentNotLoaded (document, "");
            break;
        }
    }
}
