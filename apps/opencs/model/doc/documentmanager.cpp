
#include "documentmanager.hpp"

#include <algorithm>
#include <stdexcept>

#include "document.hpp"

CSMDoc::DocumentManager::DocumentManager() {}

CSMDoc::DocumentManager::~DocumentManager()
{
    for (std::vector<Document *>::iterator iter (mDocuments.begin()); iter!=mDocuments.end(); ++iter)
        delete *iter;
}

CSMDoc::Document *CSMDoc::DocumentManager::addDocument (const std::vector<boost::filesystem::path>& files, const boost::filesystem::path& savePath,
    bool new_)
{
    Document *document = new Document (files, savePath, new_);

    mDocuments.push_back (document);

    return document;
}

bool CSMDoc::DocumentManager::removeDocument (Document *document)
{
    std::vector<Document *>::iterator iter = std::find (mDocuments.begin(), mDocuments.end(), document);

    if (iter==mDocuments.end())
            throw std::runtime_error ("removing invalid document");

    mDocuments.erase (iter);
    delete document;

    return mDocuments.empty();
}