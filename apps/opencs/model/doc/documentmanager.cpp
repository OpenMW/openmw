
#include "documentmanager.hpp"

#include <algorithm>
#include <stdexcept>

#include <boost/filesystem.hpp>

#include "document.hpp"

CSMDoc::DocumentManager::DocumentManager (const boost::filesystem::path& projectPath)
: mProjectPath (projectPath)
{
    if (!boost::filesystem::is_directory (mProjectPath))
        boost::filesystem::create_directories (mProjectPath);
}

CSMDoc::DocumentManager::~DocumentManager()
{
    for (std::vector<Document *>::iterator iter (mDocuments.begin()); iter!=mDocuments.end(); ++iter)
        delete *iter;
}

CSMDoc::Document *CSMDoc::DocumentManager::addDocument (const std::vector<boost::filesystem::path>& files, const boost::filesystem::path& savePath,
    bool new_)
{
    boost::filesystem::path projectFile (mProjectPath);

    projectFile /= savePath.filename().string() + ".project";

    Document *document = new Document (files, savePath, new_, projectFile);

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