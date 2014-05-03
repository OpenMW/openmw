
#include "loader.hpp"

#include <QTimer>

#include "document.hpp"

CSMDoc::Loader::Stage::Stage() : mFile (0) {}


CSMDoc::Loader::Loader()
{
    QTimer *timer = new QTimer (this);

    connect (timer, SIGNAL (timeout()), this, SLOT (load()));
    timer->start (1000);
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
        if (iter->second.mFile<size)
        {
            document->getData().loadFile (document->getContentFiles()[iter->second.mFile],
                iter->second.mFile<size-1, false);
        }
        else if (iter->second.mFile==size)
        {
            document->getData().loadFile (document->getProjectPath(), false, true);
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