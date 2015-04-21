
#include "documentmanager.hpp"

#include <algorithm>
#include <stdexcept>

#include <boost/filesystem.hpp>

#ifndef Q_MOC_RUN
#include <components/files/configurationmanager.hpp>
#endif

#include "document.hpp"

CSMDoc::DocumentManager::DocumentManager (const Files::ConfigurationManager& configuration)
: mConfiguration (configuration), mEncoding (ToUTF8::WINDOWS_1252)
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
    connect (this, SIGNAL (loadRequest (CSMDoc::Document *)),
        &mLoader, SLOT (loadDocument (CSMDoc::Document *)));
    connect (&mLoader, SIGNAL (nextStage (CSMDoc::Document *, const std::string&, int)),
        this, SIGNAL (nextStage (CSMDoc::Document *, const std::string&, int)));
    connect (&mLoader, SIGNAL (nextRecord (CSMDoc::Document *, int)),
        this, SIGNAL (nextRecord (CSMDoc::Document *, int)));
    connect (this, SIGNAL (cancelLoading (CSMDoc::Document *)),
        &mLoader, SLOT (abortLoading (CSMDoc::Document *)));
    connect (&mLoader, SIGNAL (loadMessage (CSMDoc::Document *, const std::string&)),
        this, SIGNAL (loadMessage (CSMDoc::Document *, const std::string&)));
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
    Document *document = new Document (mConfiguration, files, new_, savePath, mResDir, mEncoding, mResourcesManager, mBlacklistedScripts);

    mDocuments.push_back (document);

    emit loadRequest (document);

    mLoader.hasThingsToDo().wakeAll();
}

void CSMDoc::DocumentManager::removeDocument (CSMDoc::Document *document)
{
    std::vector<Document *>::iterator iter = std::find (mDocuments.begin(), mDocuments.end(), document);

    if (iter==mDocuments.end())
        throw std::runtime_error ("removing invalid document");

    mDocuments.erase (iter);
    document->deleteLater();

    if (mDocuments.empty())
        emit lastDocumentDeleted();
}

void CSMDoc::DocumentManager::setResourceDir (const boost::filesystem::path& parResDir)
{
    mResDir = boost::filesystem::system_complete(parResDir);
}

void CSMDoc::DocumentManager::setEncoding (ToUTF8::FromType encoding)
{
    mEncoding = encoding;
}

void CSMDoc::DocumentManager::setBlacklistedScripts (const std::vector<std::string>& scriptIds)
{
    mBlacklistedScripts = scriptIds;
}

void CSMDoc::DocumentManager::listResources()
{
    mResourcesManager.listResources();
}

void CSMDoc::DocumentManager::documentLoaded (Document *document)
{
    emit documentAdded (document);
    emit loadingStopped (document, true, "");
}

void CSMDoc::DocumentManager::documentNotLoaded (Document *document, const std::string& error)
{
    emit loadingStopped (document, false, error);

    if (error.empty()) // do not remove the document yet, if we have an error
        removeDocument (document);
}
