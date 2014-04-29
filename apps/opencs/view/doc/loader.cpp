
#include "loader.hpp"

#include "../../model/doc/document.hpp"

CSVDoc::LoadingDocument::LoadingDocument (CSMDoc::Document *document)
{
    setWindowTitle (("Loading " + document->getSavePath().filename().string()).c_str());
    show();
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