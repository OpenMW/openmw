#include "documentmanager.hpp"

#include <boost/filesystem.hpp>

#ifndef Q_MOC_RUN
#include <components/files/configurationmanager.hpp>
#endif

#include "document.hpp"

CSMDoc::DocumentManager::DocumentManager (const Files::ConfigurationManager& configuration)
: mConfiguration (configuration), mEncoding (ToUTF8::WINDOWS_1252), mFsStrict(false)
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
    mLoader.stop();
    mLoader.hasThingsToDo().wakeAll();
    mLoaderThread.wait();

    for (std::vector<Document *>::iterator iter (mDocuments.begin()); iter!=mDocuments.end(); ++iter)
        delete *iter;
}

bool CSMDoc::DocumentManager::isEmpty()
{
    return mDocuments.empty();
}

void CSMDoc::DocumentManager::addDocument (const std::vector<boost::filesystem::path>& files, const boost::filesystem::path& savePath,
    bool new_)
{
    Document *document = makeDocument (files, savePath, new_);
    insertDocument (document);
}

CSMDoc::Document *CSMDoc::DocumentManager::makeDocument (
    const std::vector< boost::filesystem::path >& files,
    const boost::filesystem::path& savePath, bool new_)
{
    return new Document (mConfiguration, files, new_, savePath, mResDir, mEncoding, mBlacklistedScripts, mFsStrict, mDataPaths, mArchives);
}

void CSMDoc::DocumentManager::insertDocument (CSMDoc::Document *document)
{
    mDocuments.push_back (document);

    connect (document, SIGNAL (mergeDone (CSMDoc::Document*)),
        this, SLOT (insertDocument (CSMDoc::Document*)));

    emit loadRequest (document);

    mLoader.hasThingsToDo().wakeAll();
}

void CSMDoc::DocumentManager::removeDocument (CSMDoc::Document *document)
{
    std::vector<Document *>::iterator iter = std::find (mDocuments.begin(), mDocuments.end(), document);

    if (iter==mDocuments.end())
        throw std::runtime_error ("removing invalid document");

    emit documentAboutToBeRemoved (document);

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

void CSMDoc::DocumentManager::setFileData(bool strict, const Files::PathContainer& dataPaths, const std::vector<std::string>& archives)
{
    mFsStrict = strict;
    mDataPaths = dataPaths;
    mArchives = archives;
}
