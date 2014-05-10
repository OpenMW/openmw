
#include "loader.hpp"

#include <QTimer>

#include "../tools/reportmodel.hpp"

#include "document.hpp"
#include "state.hpp"

CSMDoc::Loader::Stage::Stage() : mFile (0), mRecordsLeft (false) {}


CSMDoc::Loader::Loader()
{
    QTimer *timer = new QTimer (this);

    connect (timer, SIGNAL (timeout()), this, SLOT (load()));
    timer->start();
}

QWaitCondition& CSMDoc::Loader::hasThingsToDo()
{
    return mThingsToDo;
}

void CSMDoc::Loader::load()
{
    if (mDocuments.empty())
    {
        mMutex.lock();
        mThingsToDo.wait (&mMutex);
        mMutex.unlock();
        return;
    }

    std::vector<std::pair<Document *, Stage> >::iterator iter = mDocuments.begin();

    Document *document = iter->first;

    int size = static_cast<int> (document->getContentFiles().size());

    if (document->isNew())
        --size;

    bool done = false;

    const int batchingSize = 100;

    try
    {
        if (iter->second.mRecordsLeft)
        {
            CSMDoc::Stage::Messages messages;
            for (int i=0; i<batchingSize; ++i) // do not flood the system with update signals
                if (document->getData().continueLoading (messages))
                {
                    iter->second.mRecordsLeft = false;
                    break;
                }

            CSMWorld::UniversalId log (CSMWorld::UniversalId::Type_LoadErrorLog, 0);

            for (CSMDoc::Stage::Messages::const_iterator iter (messages.begin());
                iter!=messages.end(); ++iter)
            {
                document->getReport (log)->add (iter->first, iter->second);
                emit loadMessage (document, iter->second);
            }

            emit nextRecord (document);

            return;
        }

        if (iter->second.mFile<size)
        {
            boost::filesystem::path path = document->getContentFiles()[iter->second.mFile];

            int steps = document->getData().startLoading (path, iter->second.mFile<size-1, false);
            iter->second.mRecordsLeft = true;

            emit nextStage (document, path.filename().string(), steps/batchingSize);
        }
        else if (iter->second.mFile==size)
        {
            int steps = document->getData().startLoading (document->getProjectPath(), false, true);
            iter->second.mRecordsLeft = true;

            emit nextStage (document, "Project File", steps/batchingSize);
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
    mDocuments.push_back (std::make_pair (document, Stage()));
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
