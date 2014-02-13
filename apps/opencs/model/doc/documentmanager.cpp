
#include "documentmanager.hpp"

#include <algorithm>
#include <stdexcept>

#include <boost/filesystem.hpp>

#ifndef Q_MOC_RUN
#include <components/files/configurationmanager.hpp>
#endif

#include "document.hpp"

CSMDoc::DocumentManager::DocumentManager (const Files::ConfigurationManager& configuration)
: mConfiguration (configuration)
{
    boost::filesystem::path projectPath = configuration.getUserDataPath() / "projects";

    if (!boost::filesystem::is_directory (projectPath))
        boost::filesystem::create_directories (projectPath);
}

CSMDoc::DocumentManager::~DocumentManager()
{
    for (std::vector<Document *>::iterator iter (mDocuments.begin()); iter!=mDocuments.end(); ++iter)
        delete *iter;
}

CSMDoc::Document *CSMDoc::DocumentManager::addDocument (const std::vector<boost::filesystem::path>& files, const boost::filesystem::path& savePath,
    bool new_)
{
    Document *document = new Document (mConfiguration, files, savePath, mResDir, new_);

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

void CSMDoc::DocumentManager::setResourceDir (const boost::filesystem::path& parResDir)
{
    mResDir = boost::filesystem::system_complete(parResDir);
}
