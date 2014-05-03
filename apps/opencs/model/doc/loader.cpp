
#include "loader.hpp"

#include <QTimer>

#include "document.hpp"

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

    try
    {
        if (iter->second.mRecordsLeft)
        {
            if (document->getData().continueLoading())
                iter->second.mRecordsLeft = false;

            return;
        }

        if (iter->second.mFile<size)
        {
            boost::filesystem::path path = document->getContentFiles()[iter->second.mFile];

            emit nextStage (document, path.filename().string());

            document->getData().startLoading (path, iter->second.mFile<size-1, false);
            iter->second.mRecordsLeft = true;
        }
        else if (iter->second.mFile==size)
        {
            emit nextStage (document, "Project File");

            document->getData().startLoading (document->getProjectPath(), false, true);
            iter->second.mRecordsLeft = true;
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

void CSMDoc::Loader::abortLoading (Document *document)
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