#include "loader.hpp"

#include <QVBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QCursor>
#include <QDialogButtonBox>
#include <QCloseEvent>
#include <QListWidget>

#include "../../model/doc/document.hpp"

void CSVDoc::LoadingDocument::closeEvent (QCloseEvent *event)
{
    event->ignore();
    cancel();
}

CSVDoc::LoadingDocument::LoadingDocument (CSMDoc::Document *document)
: mDocument (document), mTotalRecordsLabel (0), mRecordsLabel (0), mAborted (false), mMessages (nullptr), mRecords(0)
{
    setWindowTitle (QString::fromUtf8((std::string("Opening ") + document->getSavePath().filename().string()).c_str()));

    setMinimumWidth (400);

    mLayout = new QVBoxLayout (this);

    // total progress
    mTotalRecordsLabel = new QLabel (this);

    mLayout->addWidget (mTotalRecordsLabel);

    mTotalProgress = new QProgressBar (this);

    mLayout->addWidget (mTotalProgress);

    mTotalProgress->setMinimum (0);
    mTotalProgress->setMaximum (document->getData().getTotalRecords(document->getContentFiles()));
    mTotalProgress->setTextVisible (true);
    mTotalProgress->setValue (0);
    mTotalRecords = 0;

    mFilesLoaded = 0;

    // record progress
    mLayout->addWidget (mRecordsLabel = new QLabel ("Records", this));

    mRecordProgress = new QProgressBar (this);

    mLayout->addWidget (mRecordProgress);

    mRecordProgress->setMinimum (0);
    mRecordProgress->setTextVisible (true);
    mRecordProgress->setValue (0);

    // error message
    mError = new QLabel (this);
    mError->setWordWrap (true);

    mLayout->addWidget (mError);

    // buttons
    mButtons = new QDialogButtonBox (QDialogButtonBox::Cancel, Qt::Horizontal, this);

    mLayout->addWidget (mButtons);

    setLayout (mLayout);

    move (QCursor::pos());

    show();

    connect (mButtons, SIGNAL (rejected()), this, SLOT (cancel()));
}

void CSVDoc::LoadingDocument::nextStage (const std::string& name, int fileRecords)
{
    ++mFilesLoaded;
    size_t numFiles = mDocument->getContentFiles().size();

    mTotalRecordsLabel->setText (QString::fromUtf8 (("Loading: "+name
                    +" ("+std::to_string(mFilesLoaded)+" of "+std::to_string((numFiles))+")").c_str()));

    mTotalRecords = mTotalProgress->value();

    mRecordProgress->setValue (0);
    mRecordProgress->setMaximum (fileRecords>0 ? fileRecords : 1);

    mRecords = fileRecords;
}

void CSVDoc::LoadingDocument::nextRecord (int records)
{
    if (records <= mRecords)
    {
        mTotalProgress->setValue (mTotalRecords+records);

        mRecordProgress->setValue(records);

        mRecordsLabel->setText(QString::fromStdString(
                    "Records: "+std::to_string(records)+" of "+std::to_string(mRecords)));
    }
}

void CSVDoc::LoadingDocument::abort (const std::string& error)
{
    mAborted = true;
    mError->setText (QString::fromUtf8 (("<font color=red>Loading failed: " + error + "</font>").c_str()));
    mButtons->setStandardButtons (QDialogButtonBox::Close);
}

void CSVDoc::LoadingDocument::addMessage (const std::string& message)
{
    if (!mMessages)
    {
        mMessages = new QListWidget (this);
        mLayout->insertWidget (4, mMessages);
    }

    new QListWidgetItem (QString::fromUtf8 (message.c_str()), mMessages);
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
    else
    {
        iter->second->abort (error);
        // Leave the window open for now (wait for the user to close it)
        mDocuments.erase (iter);
    }
}

void CSVDoc::Loader::nextStage (CSMDoc::Document *document, const std::string& name,
    int fileRecords)
{
    std::map<CSMDoc::Document *, LoadingDocument *>::iterator iter = mDocuments.find (document);

    if (iter!=mDocuments.end())
        iter->second->nextStage (name, fileRecords);
}

void CSVDoc::Loader::nextRecord (CSMDoc::Document *document, int records)
{
    std::map<CSMDoc::Document *, LoadingDocument *>::iterator iter = mDocuments.find (document);

    if (iter!=mDocuments.end())
        iter->second->nextRecord (records);
}

void CSVDoc::Loader::loadMessage (CSMDoc::Document *document, const std::string& message)
{
    std::map<CSMDoc::Document *, LoadingDocument *>::iterator iter = mDocuments.find (document);

    if (iter!=mDocuments.end())
        iter->second->addMessage (message);
}
