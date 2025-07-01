#include "editor.hpp"

#include <QApplication>
#include <QFileInfo>
#include <QLocalServer>
#include <QLocalSocket>
#include <QMessageBox>
#include <QRegularExpression>

#include <boost/program_options.hpp>

#include <exception>

#include <apps/opencs/model/doc/document.hpp>
#include <apps/opencs/model/doc/documentmanager.hpp>
#include <apps/opencs/view/doc/adjusterwidget.hpp>
#include <apps/opencs/view/doc/filedialog.hpp>
#include <apps/opencs/view/doc/newgame.hpp>
#include <apps/opencs/view/doc/startup.hpp>
#include <apps/opencs/view/prefs/dialogue.hpp>
#include <apps/opencs/view/tools/merge.hpp>

#ifdef _WIN32
#include <components/misc/windows.hpp>
#endif

#include <components/debug/debugging.hpp>
#include <components/debug/debuglog.hpp>
#include <components/esm3/esmreader.hpp>
#include <components/esm3/loadtes3.hpp>
#include <components/fallback/fallback.hpp>
#include <components/fallback/validate.hpp>
#include <components/files/qtconversion.hpp>
#include <components/misc/rng.hpp>
#include <components/nifosg/nifloader.hpp>
#include <components/settings/settings.hpp>
#include <components/toutf8/toutf8.hpp>

#include "view/doc/viewmanager.hpp"

CS::Editor::Editor(int argc, char** argv)
    : mConfigVariables(readConfiguration())
    , mSettingsState(mCfgMgr)
    , mDocumentManager(mCfgMgr)
    , mPid(std::filesystem::temp_directory_path() / "openmw-cs.pid")
    , mLockFile(QFileInfo(Files::pathToQString(mPid)).absoluteFilePath() + ".lock")
    , mMerge(mDocumentManager)
    , mIpcServerName("org.openmw.OpenCS")
    , mServer(nullptr)
    , mClientSocket(nullptr)
{
    std::pair<Files::PathContainer, std::vector<std::string>> config = readConfig();

    mViewManager = new CSVDoc::ViewManager(mDocumentManager);
    if (argc > 1)
    {
        mFileToLoad = argv[1];
        mDataDirs = config.first;
    }

    NifOsg::Loader::setShowMarkers(true);

    mDocumentManager.setFileData(config.first, config.second);

    mNewGame.setLocalData(mLocal);
    mFileDialog.setLocalData(mLocal);
    mMerge.setLocalData(mLocal);

    connect(&mDocumentManager, &CSMDoc::DocumentManager::documentAdded, this, &Editor::documentAdded);
    connect(
        &mDocumentManager, &CSMDoc::DocumentManager::documentAboutToBeRemoved, this, &Editor::documentAboutToBeRemoved);
    connect(&mDocumentManager, &CSMDoc::DocumentManager::lastDocumentDeleted, this, &Editor::lastDocumentDeleted);

    connect(mViewManager, &CSVDoc::ViewManager::newGameRequest, this, &Editor::createGame);
    connect(mViewManager, &CSVDoc::ViewManager::newAddonRequest, this, &Editor::createAddon);
    connect(mViewManager, &CSVDoc::ViewManager::loadDocumentRequest, this, &Editor::loadDocument);
    connect(mViewManager, &CSVDoc::ViewManager::editSettingsRequest, this, &Editor::showSettings);
    connect(mViewManager, &CSVDoc::ViewManager::mergeDocument, this, &Editor::mergeDocument);

    connect(&mStartup, &CSVDoc::StartupDialogue::createGame, this, &Editor::createGame);
    connect(&mStartup, &CSVDoc::StartupDialogue::createAddon, this, &Editor::createAddon);
    connect(&mStartup, &CSVDoc::StartupDialogue::loadDocument, this, &Editor::loadDocument);
    connect(&mStartup, &CSVDoc::StartupDialogue::editConfig, this, &Editor::showSettings);

    connect(&mFileDialog, &CSVDoc::FileDialog::signalOpenFiles, this,
        [this](const std::filesystem::path& savePath) { this->openFiles(savePath); });
    connect(&mFileDialog, &CSVDoc::FileDialog::signalCreateNewFile, this, &Editor::createNewFile);
    connect(&mFileDialog, &CSVDoc::FileDialog::rejected, this, &Editor::cancelFileDialog);

    connect(&mNewGame, &CSVDoc::NewGameDialogue::createRequest, this, &Editor::createNewGame);
    connect(&mNewGame, &CSVDoc::NewGameDialogue::cancelCreateGame, this, &Editor::cancelCreateGame);
}

CS::Editor::~Editor()
{
    delete mViewManager;

    mLockFile.unlock();
    mPidFile.close();

    if (mServer && std::filesystem::exists(mPid))
        std::filesystem::remove(mPid);
}

boost::program_options::variables_map CS::Editor::readConfiguration()
{
    boost::program_options::variables_map variables;
    boost::program_options::options_description desc("Syntax: openmw-cs <options>\nAllowed options");

    auto addOption = desc.add_options();
    addOption("data",
        boost::program_options::value<Files::MaybeQuotedPathContainer>()
            ->default_value(Files::MaybeQuotedPathContainer(), "data")
            ->multitoken()
            ->composing());
    addOption("data-local",
        boost::program_options::value<Files::MaybeQuotedPathContainer::value_type>()->default_value(
            Files::MaybeQuotedPathContainer::value_type(), ""));
    addOption("encoding", boost::program_options::value<std::string>()->default_value("win1252"));
    addOption("fallback-archive",
        boost::program_options::value<std::vector<std::string>>()
            ->default_value(std::vector<std::string>(), "fallback-archive")
            ->multitoken());
    addOption("fallback",
        boost::program_options::value<Fallback::FallbackMap>()
            ->default_value(Fallback::FallbackMap(), "")
            ->multitoken()
            ->composing(),
        "fallback values");
    Files::ConfigurationManager::addCommonOptions(desc);

    boost::program_options::notify(variables);

    mCfgMgr.readConfiguration(variables, desc, false);
    Settings::Manager::load(mCfgMgr, true);
    Debug::setupLogging(mCfgMgr.getLogPath(), "OpenMW-CS");

    return variables;
}

std::pair<Files::PathContainer, std::vector<std::string>> CS::Editor::readConfig(bool quiet)
{
    boost::program_options::variables_map& variables = mConfigVariables;

    Fallback::Map::init(variables["fallback"].as<Fallback::FallbackMap>().mMap);

    mEncodingName = variables["encoding"].as<std::string>();
    mDocumentManager.setEncoding(ToUTF8::calculateEncoding(mEncodingName));
    mFileDialog.setEncoding(QString::fromUtf8(mEncodingName.c_str()));

    mDocumentManager.setResourceDir(mResources = variables["resources"]
                                                     .as<Files::MaybeQuotedPath>()
                                                     .u8string()); // This call to u8string is redundant, but required
                                                                   // to build on MSVC 14.26 due to implementation bugs.

    Files::PathContainer dataDirs, dataLocal;
    if (!variables["data"].empty())
    {
        dataDirs = asPathContainer(variables["data"].as<Files::MaybeQuotedPathContainer>());
    }

    Files::PathContainer::value_type local(variables["data-local"]
                                               .as<Files::MaybeQuotedPathContainer::value_type>()
                                               .u8string()); // This call to u8string is redundant, but required to
                                                             // build on MSVC 14.26 due to implementation bugs.
    if (!local.empty())
    {
        std::filesystem::create_directories(local);
        dataLocal.push_back(local);
    }
    mCfgMgr.filterOutNonExistingPaths(dataDirs);
    mCfgMgr.filterOutNonExistingPaths(dataLocal);

    if (!dataLocal.empty())
        mLocal = dataLocal[0];
    else
    {
        QMessageBox messageBox;
        messageBox.setWindowTitle(tr("No local data path available"));
        messageBox.setIcon(QMessageBox::Critical);
        messageBox.setStandardButtons(QMessageBox::Ok);
        messageBox.setText(
            tr("<br><b>OpenCS is unable to access the local data directory. This may indicate a faulty configuration "
               "or a broken install.</b>"));
        messageBox.exec();

        QApplication::exit(1);
    }

    dataDirs.insert(dataDirs.end(), dataLocal.begin(), dataLocal.end());

    dataDirs.insert(dataDirs.begin(), mResources / "vfs");

    // iterate the data directories and add them to the file dialog for loading
    mFileDialog.addFiles(dataDirs);

    return std::make_pair(dataDirs, variables["fallback-archive"].as<std::vector<std::string>>());
}

void CS::Editor::createGame()
{
    mStartup.hide();

    if (mNewGame.isHidden())
        mNewGame.show();

    mNewGame.raise();
    mNewGame.activateWindow();
}

void CS::Editor::cancelCreateGame()
{
    if (!mDocumentManager.isEmpty())
        return;

    mNewGame.hide();

    if (mStartup.isHidden())
        mStartup.show();

    mStartup.raise();
    mStartup.activateWindow();
}

void CS::Editor::createAddon()
{
    mStartup.hide();

    mFileDialog.clearFiles();
    readConfig(/*quiet*/ true);

    mFileDialog.showDialog(CSVDoc::ContentAction_New);
}

void CS::Editor::cancelFileDialog()
{
    if (!mDocumentManager.isEmpty())
        return;

    mFileDialog.hide();

    if (mStartup.isHidden())
        mStartup.show();

    mStartup.raise();
    mStartup.activateWindow();
}

void CS::Editor::loadDocument()
{
    mStartup.hide();

    mFileDialog.clearFiles();
    readConfig(/*quiet*/ true);

    mFileDialog.showDialog(CSVDoc::ContentAction_Edit);
}

void CS::Editor::openFiles(
    const std::filesystem::path& savePath, const std::vector<std::filesystem::path>& discoveredFiles)
{
    std::vector<std::filesystem::path> files;

    if (discoveredFiles.empty())
    {
        for (const QString& path : mFileDialog.selectedFilePaths())
        {
            files.emplace_back(Files::pathFromQString(path));
        }
    }
    else
    {
        files = discoveredFiles;
    }

    mDocumentManager.addDocument(files, savePath, false);

    mFileDialog.hide();
}

void CS::Editor::createNewFile(const std::filesystem::path& savePath)
{
    std::vector<std::filesystem::path> files;

    for (const QString& path : mFileDialog.selectedFilePaths())
    {
        files.emplace_back(Files::pathFromQString(path));
    }

    files.push_back(savePath);

    mDocumentManager.addDocument(files, savePath, true);

    mFileDialog.hide();
}

void CS::Editor::createNewGame(const std::filesystem::path& file)
{
    std::vector<std::filesystem::path> files;

    files.push_back(file);

    mDocumentManager.addDocument(files, file, true);

    mNewGame.hide();
}

void CS::Editor::showStartup()
{
    if (mStartup.isHidden())
        mStartup.show();
    mStartup.raise();
    mStartup.activateWindow();
}

void CS::Editor::showSettings()
{
    if (mSettings.isHidden())
        mSettings.show();

    mSettings.move(QCursor::pos());
    mSettings.raise();
    mSettings.activateWindow();
}

bool CS::Editor::makeIPCServer()
{
    try
    {
        bool pidExists = std::filesystem::exists(mPid);

        mPidFile.open(mPid);

        if (!mLockFile.tryLock())
        {
            Log(Debug::Error) << "Error: OpenMW-CS is already running.";
            return false;
        }

#ifdef _WIN32
        mPidFile << GetCurrentProcessId() << std::endl;
#else
        mPidFile << getpid() << std::endl;
#endif

        mServer = new QLocalServer(this);

        if (pidExists)
        {
            // hack to get the temp directory path
            mServer->listen("dummy");
            QString fullPath = mServer->fullServerName();
            mServer->close();
            fullPath.remove(QRegularExpression("dummy$"));
            fullPath += mIpcServerName;
            const auto path = Files::pathFromQString(fullPath);
            if (exists(path))
            {
                // TODO: compare pid of the current process with that in the file
                Log(Debug::Info) << "Detected unclean shutdown.";
                // delete the stale file
                if (remove(path))
                    Log(Debug::Error) << "Error: can not remove stale connection file.";
            }
        }
    }

    catch (const std::exception& e)
    {
        Log(Debug::Error) << "Error: " << e.what();
        return false;
    }

    if (mServer->listen(mIpcServerName))
    {
        connect(mServer, &QLocalServer::newConnection, this, &Editor::showStartup);
        return true;
    }

    mServer->close();
    mServer = nullptr;
    return false;
}

void CS::Editor::connectToIPCServer()
{
    mClientSocket = new QLocalSocket(this);
    mClientSocket->connectToServer(mIpcServerName);
    mClientSocket->close();
}

int CS::Editor::run()
{
    if (mLocal.empty())
        return 1;

    Misc::Rng::init();

    QApplication::setQuitOnLastWindowClosed(true);

    if (mFileToLoad.empty())
    {
        mStartup.show();
    }
    else
    {
        ESM::ESMReader fileReader;
        ToUTF8::Utf8Encoder encoder(ToUTF8::calculateEncoding(mEncodingName));
        fileReader.setEncoder(&encoder);
        fileReader.open(mFileToLoad);

        std::vector<std::filesystem::path> discoveredFiles;

        for (const auto& item : fileReader.getGameFiles())
        {
            for (const auto& path : mDataDirs)
            {
                if (auto masterPath = path / item.name; std::filesystem::exists(masterPath))
                {
                    discoveredFiles.emplace_back(std::move(masterPath));
                    break;
                }
            }
        }
        discoveredFiles.push_back(mFileToLoad);

        const auto extension = Files::pathToQString(mFileToLoad.extension()).toLower();
        if (extension == ".esm")
        {
            mFileToLoad.replace_extension(".omwgame");
            mDocumentManager.addDocument(discoveredFiles, mFileToLoad, false);
        }
        else if (extension == ".esp")
        {
            mFileToLoad.replace_extension(".omwaddon");
            mDocumentManager.addDocument(discoveredFiles, mFileToLoad, false);
        }
        else
        {
            openFiles(mFileToLoad, discoveredFiles);
        }
    }

    return QApplication::exec();
}

void CS::Editor::documentAdded(CSMDoc::Document* document)
{
    mViewManager->addView(document);
}

void CS::Editor::documentAboutToBeRemoved(CSMDoc::Document* document)
{
    if (mMerge.getDocument() == document)
        mMerge.cancel();
}

void CS::Editor::lastDocumentDeleted()
{
    QApplication::quit();
}

void CS::Editor::mergeDocument(CSMDoc::Document* document)
{
    mMerge.configure(document);
    mMerge.show();
    mMerge.raise();
    mMerge.activateWindow();
}
