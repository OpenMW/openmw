#include "documentmanager.hpp"

#include <QWaitCondition>

#include <algorithm>
#include <filesystem>
#include <stdexcept>

#include <apps/opencs/model/doc/loader.hpp>

#ifndef Q_MOC_RUN
#include <components/files/configurationmanager.hpp>
#endif

#include "document.hpp"

CSMDoc::DocumentManager::DocumentManager(const Files::ConfigurationManager& configuration)
    : mConfiguration(configuration)
    , mEncoding(ToUTF8::WINDOWS_1252)
{
    std::filesystem::path projectPath = configuration.getUserDataPath() / "projects";

    if (!std::filesystem::is_directory(projectPath))
        std::filesystem::create_directories(projectPath);

    mLoader.moveToThread(&mLoaderThread);
    mLoaderThread.start();

    connect(&mLoader, &Loader::documentLoaded, this, &DocumentManager::documentLoaded);
    connect(&mLoader, &Loader::documentNotLoaded, this, &DocumentManager::documentNotLoaded);
    connect(this, &DocumentManager::loadRequest, &mLoader, &Loader::loadDocument);
    connect(&mLoader, &Loader::nextStage, this, &DocumentManager::nextStage);
    connect(&mLoader, &Loader::nextRecord, this, &DocumentManager::nextRecord);
    connect(this, &DocumentManager::cancelLoading, &mLoader, &Loader::abortLoading);
    connect(&mLoader, &Loader::loadMessage, this, &DocumentManager::loadMessage);
}

CSMDoc::DocumentManager::~DocumentManager()
{
    mLoaderThread.quit();
    mLoader.stop();
    mLoader.hasThingsToDo().wakeAll();
    mLoaderThread.wait();

    for (std::vector<Document*>::iterator iter(mDocuments.begin()); iter != mDocuments.end(); ++iter)
        delete *iter;
}

bool CSMDoc::DocumentManager::isEmpty()
{
    return mDocuments.empty();
}

void CSMDoc::DocumentManager::addDocument(
    const std::vector<std::filesystem::path>& files, const std::filesystem::path& savePath, bool isNew)
{
    Document* document = makeDocument(files, savePath, isNew);
    insertDocument(document);
}

CSMDoc::Document* CSMDoc::DocumentManager::makeDocument(
    const std::vector<std::filesystem::path>& files, const std::filesystem::path& savePath, bool isNew)
{
    return new Document(mConfiguration, files, isNew, savePath, mResDir, mEncoding, mDataPaths, mArchives);
}

void CSMDoc::DocumentManager::insertDocument(CSMDoc::Document* document)
{
    mDocuments.push_back(document);

    connect(document, SIGNAL(mergeDone(CSMDoc::Document*)), this, SLOT(insertDocument(CSMDoc::Document*)));

    emit loadRequest(document);

    mLoader.hasThingsToDo().wakeAll();
}

void CSMDoc::DocumentManager::removeDocument(CSMDoc::Document* document)
{
    std::vector<Document*>::iterator iter = std::find(mDocuments.begin(), mDocuments.end(), document);

    if (iter == mDocuments.end())
        throw std::runtime_error("removing invalid document");

    emit documentAboutToBeRemoved(document);

    mDocuments.erase(iter);
    document->deleteLater();

    if (mDocuments.empty())
        emit lastDocumentDeleted();
}

void CSMDoc::DocumentManager::setResourceDir(const std::filesystem::path& parResDir)
{
    mResDir = std::filesystem::absolute(parResDir);
}

void CSMDoc::DocumentManager::setEncoding(ToUTF8::FromType encoding)
{
    mEncoding = encoding;
}

void CSMDoc::DocumentManager::documentLoaded(Document* document)
{
    emit documentAdded(document);
    emit loadingStopped(document, true, "");
}

void CSMDoc::DocumentManager::documentNotLoaded(Document* document, const std::string& error)
{
    emit loadingStopped(document, false, error);

    if (error.empty()) // do not remove the document yet, if we have an error
        removeDocument(document);
}

void CSMDoc::DocumentManager::setFileData(
    const Files::PathContainer& dataPaths, const std::vector<std::string>& archives)
{
    mDataPaths = dataPaths;
    mArchives = archives;
}
