
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

    mLoader.moveToThread (&mLoaderThread);
    mLoaderThread.start();

    connect (&mLoader, SIGNAL (documentLoaded (Document *)),
        this, SLOT (documentLoaded (Document *)));
    connect (&mLoader, SIGNAL (documentNotLoaded (Document *, const std::string&)),
        this, SLOT (documentNotLoaded (Document *, const std::string&)));
    connect (this, SIGNAL (loadRequest (Document *, bool)),
        &mLoader, SLOT (loadDocument (Document *, bool)));
}

CSMDoc::DocumentManager::~DocumentManager()
{
    mLoaderThread.quit();
    mLoader.hasThingsToDo().wakeAll();
    mLoaderThread.wait();

    for (std::vector<Document *>::iterator iter (mDocuments.begin()); iter!=mDocuments.end(); ++iter)
        delete *iter;
}

void CSMDoc::DocumentManager::addDocument (const std::vector<boost::filesystem::path>& files, const boost::filesystem::path& savePath,
    bool new_)
{
    Document *document = new Document (mConfiguration, files, savePath, mResDir);

    mDocuments.push_back (document);

    emit loadRequest (document, new_);

    mLoader.hasThingsToDo().wakeAll();
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

void CSMDoc::DocumentManager::documentLoaded (Document *document)
{
    emit documentAdded (document);
}

void CSMDoc::DocumentManager::documentNotLoaded (Document *document, const std::string& error)
{
    removeDocument (document);
    /// \todo report error
    /// \todo handle removeDocument returning true
}