
#include "loader.hpp"

#include <QVBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QCursor>
#include <QDialogButtonBox>
#include <QCloseEvent>

#include "../../model/doc/document.hpp"

void CSVDoc::LoadingDocument::closeEvent (QCloseEvent *event)
{
    event->ignore();
    cancel();
}

CSVDoc::LoadingDocument::LoadingDocument (CSMDoc::Document *document)
: mDocument (document), mAborted (false)
{
    setWindowTitle (("Opening " + document->getSavePath().filename().string()).c_str());

    QVBoxLayout *layout = new QVBoxLayout (this);

    // file progress
    mFile = new QLabel (this);

    layout->addWidget (mFile);

    mFileProgress = new QProgressBar (this);

    layout->addWidget (mFileProgress);

    int size = static_cast<int> (document->getContentFiles().size())+1;
    if (document->isNew())
        --size;

    mFileProgress->setMinimum (0);
    mFileProgress->setMaximum (size);
    mFileProgress->setTextVisible (true);
    mFileProgress->setValue (0);

    // record progress
    layout->addWidget (new QLabel ("Records", this));

    mRecordProgress = new QProgressBar (this);

    layout->addWidget (mRecordProgress);

    mRecordProgress->setMinimum (0);
    mRecordProgress->setTextVisible (true);
    mRecordProgress->setValue (0);

    // error message
    mError = new QLabel (this);
    mError->setWordWrap (true);

    layout->addWidget (mError);

    // buttons
    mButtons = new QDialogButtonBox (QDialogButtonBox::Cancel, Qt::Horizontal, this);

    layout->addWidget (mButtons);

    setLayout (layout);

    move (QCursor::pos());

    show();

    connect (mButtons, SIGNAL (rejected()), this, SLOT (cancel()));
}

void CSVDoc::LoadingDocument::nextStage (const std::string& name, int steps)
{
    mFile->setText (QString::fromUtf8 (("Loading: " + name).c_str()));

    mFileProgress->setValue (mFileProgress->value()+1);

    mRecordProgress->setValue (0);
    mRecordProgress->setMaximum (steps>0 ? steps : 1);
}

void CSVDoc::LoadingDocument::nextRecord()
{
    int value = mRecordProgress->value()+1;

    if (value<=mRecordProgress->maximum())
        mRecordProgress->setValue (value);
}

void CSVDoc::LoadingDocument::abort (const std::string& error)
{
    mAborted = true;
    mError->setText (QString::fromUtf8 (("Loading failed: " + error).c_str()));
    mButtons->setStandardButtons (QDialogButtonBox::Close);
}

void CSVDoc::LoadingDocument::cancel()
{
    if (!mAborted)
        emit cancel (mDocument);
    else
    {
        emit close (mDocument);
        deleteLater();
    }
}


CSVDoc::Loader::Loader() {}

CSVDoc::Loader::~Loader()
{
    for (std::map<CSMDoc::Document *, LoadingDocument *>::iterator iter (mDocuments.begin());
        iter!=mDocuments.end(); ++iter)
        delete iter->second;
}

void CSVDoc::Loader::add (CSMDoc::Document *document)
{
    LoadingDocument *loading = new LoadingDocument (document);
    mDocuments.insert (std::make_pair (document, loading));

    connect (loading, SIGNAL (cancel (CSMDoc::Document *)),
        this, SIGNAL (cancel (CSMDoc::Document *)));
    connect (loading, SIGNAL (close (CSMDoc::Document *)),
        this, SIGNAL (close (CSMDoc::Document *)));
}

void CSVDoc::Loader::loadingStopped (CSMDoc::Document *document, bool completed,
    const std::string& error)
{
    std::map<CSMDoc::Document *, LoadingDocument *>::iterator iter = mDocuments.begin();

    for (; iter!=mDocuments.end(); ++iter)
        if (iter->first==document)
            break;

    if (iter==mDocuments.end())
        return;

    if (completed || error.empty())
    {
        delete iter->second;
        mDocuments.erase (iter);
    }
    else if (!completed && !error.empty())
    {
        iter->second->abort (error);
        // Leave the window open for now (wait for the user to close it)
        mDocuments.erase (iter);
    }
}

void CSVDoc::Loader::nextStage (CSMDoc::Document *document, const std::string& name, int steps)
{
    std::map<CSMDoc::Document *, LoadingDocument *>::iterator iter = mDocuments.find (document);

    if (iter!=mDocuments.end())
        iter->second->nextStage (name, steps);
}

void CSVDoc::Loader::nextRecord (CSMDoc::Document *document)
{
    std::map<CSMDoc::Document *, LoadingDocument *>::iterator iter = mDocuments.find (document);

    if (iter!=mDocuments.end())
        iter->second->nextRecord();
}