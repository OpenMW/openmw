
#include "loader.hpp"

#include <QTimer>

#include "document.hpp"

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

    std::vector<std::pair<Document *, bool> >::iterator iter = mDocuments.begin();

    Document *document = iter->first;

    mDocuments.erase (iter);

    try
    {
        document->setupData();
        emit documentLoaded (document);
    }
    catch (const std::exception& e)
    {
        emit documentNotLoaded (document, e.what());
    }
}

void CSMDoc::Loader::loadDocument (CSMDoc::Document *document)
{
    mDocuments.push_back (std::make_pair (document, false));
}

void CSMDoc::Loader::abortLoading (Document *document)
{
    for (std::vector<std::pair<Document *, bool> >::iterator iter = mDocuments.begin();
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