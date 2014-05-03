
#include "loader.hpp"

#include <QVBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QCursor>

#include "../../model/doc/document.hpp"

CSVDoc::LoadingDocument::LoadingDocument (CSMDoc::Document *document)
{
    setWindowTitle (("Opening " + document->getSavePath().filename().string()).c_str());

    QVBoxLayout *layout = new QVBoxLayout (this);

    mFileProgress = new QProgressBar (this);

    layout->addWidget (mFileProgress);

    int size = static_cast<int> (document->getContentFiles().size())+1;
    if (document->isNew())
        --size;

    mFileProgress->setMinimum (0);
    mFileProgress->setMaximum (size);
    mFileProgress->setTextVisible (true);
    mFileProgress->setValue (0);

    mFile = new QLabel (this);

    layout->addWidget (mFile);

    setLayout (layout);

    move (QCursor::pos());

    show();
}

void CSVDoc::LoadingDocument::nextStage (const std::string& name)
{
    mFile->setText (QString::fromUtf8 (("Loading: " + name).c_str()));

    mFileProgress->setValue (mFileProgress->value()+1);
}


CSVDoc::Loader::Loader()
{

}

CSVDoc::Loader::~Loader()
{
    for (std::map<CSMDoc::Document *, LoadingDocument *>::iterator iter (mDocuments.begin());
        iter!=mDocuments.end(); ++iter)
        delete iter->second;
}

void CSVDoc::Loader::add (CSMDoc::Document *document)
{
    mDocuments.insert (std::make_pair (document, new LoadingDocument (document)));
}

void CSVDoc::Loader::loadingStopped (CSMDoc::Document *document, bool completed,
    const std::string& error)
{
    if (completed || error.empty())
    {
        for (std::map<CSMDoc::Document *, LoadingDocument *>::iterator iter (mDocuments.begin());
            iter!=mDocuments.end(); ++iter)
            if (iter->first==document)
            {
                delete iter->second;
                mDocuments.erase (iter);
                break;
            }
    }
}

void CSVDoc::Loader::nextStage (CSMDoc::Document *document, const std::string& name)
{
    std::map<CSMDoc::Document *, LoadingDocument *>::iterator iter = mDocuments.find (document);

    if (iter!=mDocuments.end())
        iter->second->nextStage (name);
}