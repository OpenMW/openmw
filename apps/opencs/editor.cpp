#include "editor.hpp"

#include <QApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <QMessageBox>

#include <components/debug/debuglog.hpp>
#include <components/fallback/validate.hpp>
#include <components/misc/rng.hpp>
#include <components/nifosg/nifloader.hpp>

#include "model/doc/document.hpp"
#include "model/world/data.hpp"

#ifdef _WIN32
#include <Windows.h>
#endif

using namespace Fallback;

CS::Editor::Editor (int argc, char **argv)
: mSettingsState (mCfgMgr), mDocumentManager (mCfgMgr),
  mPid(""), mLock(), mMerge (mDocumentManager),
  mIpcServerName ("org.openmw.OpenCS"), mServer(nullptr), mClientSocket(nullptr)
{
    std::pair<Files::PathContainer, std::vector<std::string> > config = readConfig();

    mViewManager = new CSVDoc::ViewManager(mDocumentManager);
    if (argc > 1)
    {
        mFileToLoad = argv[1];
        mDataDirs = config.first;
    }

    NifOsg::Loader::setShowMarkers(true);

    mDocumentManager.setFileData(mFsStrict, config.first, config.second);

    mNewGame.setLocalData (mLocal);
    mFileDialog.setLocalData (mLocal);
    mMerge.setLocalData (mLocal);

    connect (&mDocumentManager, SIGNAL (documentAdded (CSMDoc::Document *)),
        this, SLOT (documentAdded (CSMDoc::Document *)));
    connect (&mDocumentManager, SIGNAL (documentAboutToBeRemoved (CSMDoc::Document *)),
        this, SLOT (documentAboutToBeRemoved (CSMDoc::Document *)));
    connect (&mDocumentManager, SIGNAL (lastDocumentDeleted()),
        this, SLOT (lastDocumentDeleted()));

    connect (mViewManager, SIGNAL (newGameRequest ()), this, SLOT (createGame ()));
    connect (mViewManager, SIGNAL (newAddonRequest ()), this, SLOT (createAddon ()));
    connect (mViewManager, SIGNAL (loadDocumentRequest ()), this, SLOT (loadDocument ()));
    connect (mViewManager, SIGNAL (editSettingsRequest()), this, SLOT (showSettings ()));
    connect (mViewManager, SIGNAL (mergeDocument (CSMDoc::Document *)), this, SLOT (mergeDocument (CSMDoc::Document *)));

    connect (&mStartup, SIGNAL (createGame()), this, SLOT (createGame ()));
    connect (&mStartup, SIGNAL (createAddon()), this, SLOT (createAddon ()));
    connect (&mStartup, SIGNAL (loadDocument()), this, SLOT (loadDocument ()));
    connect (&mStartup, SIGNAL (editConfig()), this, SLOT (showSettings ()));

    connect (&mFileDialog, SIGNAL(signalOpenFiles (const boost::filesystem::path&)),
             this, SLOT(openFiles (const boost::filesystem::path&)));

    connect (&mFileDialog, SIGNAL(signalCreateNewFile (const boost::filesystem::path&)),
             this, SLOT(createNewFile (const boost::filesystem::path&)));
    connect (&mFileDialog, SIGNAL (rejected()), this, SLOT (cancelFileDialog ()));

    connect (&mNewGame, SIGNAL (createRequest (const boost::filesystem::path&)),
             this, SLOT (createNewGame (const boost::filesystem::path&)));
    connect (&mNewGame, SIGNAL (cancelCreateGame()), this, SLOT (cancelCreateGame ()));
}

CS::Editor::~Editor ()
{
    delete mViewManager;

    mPidFile.close();

    if(mServer && boost::filesystem::exists(mPid))
        static_cast<void> ( // silence coverity warning
        remove(mPid.string().c_str())); // ignore any error
}

std::pair<Files::PathContainer, std::vector<std::string> > CS::Editor::readConfig(bool quiet)
{
    boost::program_options::variables_map variables;
    boost::program_options::options_description desc("Syntax: openmw-cs <options>\nAllowed options");

    desc.add_options()
    ("data", boost::program_options::value<Files::EscapePathContainer>()->default_value(Files::EscapePathContainer(), "data")->multitoken()->composing())
    ("data-local", boost::program_options::value<Files::EscapeHashString>()->default_value(""))
    ("fs-strict", boost::program_options::value<bool>()->implicit_value(true)->default_value(false))
    ("encoding", boost::program_options::value<Files::EscapeHashString>()->default_value("win1252"))
    ("resources", boost::program_options::value<Files::EscapeHashString>()->default_value("resources"))
    ("fallback-archive", boost::program_options::value<Files::EscapeStringVector>()->
        default_value(Files::EscapeStringVector(), "fallback-archive")->multitoken())
    ("fallback", boost::program_options::value<FallbackMap>()->default_value(FallbackMap(), "")
        ->multitoken()->composing(), "fallback values")
    ("script-blacklist", boost::program_options::value<Files::EscapeStringVector>()->default_value(Files::EscapeStringVector(), "")
        ->multitoken(), "exclude specified script from the verifier (if the use of the blacklist is enabled)")
    ("script-blacklist-use", boost::program_options::value<bool>()->implicit_value(true)
        ->default_value(true), "enable script blacklisting");

    boost::program_options::notify(variables);

    mCfgMgr.readConfiguration(variables, desc, false);

    Fallback::Map::init(variables["fallback"].as<FallbackMap>().mMap);

    mEncodingName = variables["encoding"].as<Files::EscapeHashString>().toStdString();
    mDocumentManager.setEncoding(ToUTF8::calculateEncoding(mEncodingName));
    mFileDialog.setEncoding (QString::fromUtf8(mEncodingName.c_str()));

    mDocumentManager.setResourceDir (mResources = variables["resources"].as<Files::EscapeHashString>().toStdString());

    if (variables["script-blacklist-use"].as<bool>())
        mDocumentManager.setBlacklistedScripts (
            variables["script-blacklist"].as<Files::EscapeStringVector>().toStdStringVector());

    mFsStrict = variables["fs-strict"].as<bool>();

    Files::PathContainer dataDirs, dataLocal;
    if (!variables["data"].empty()) {
        dataDirs = Files::PathContainer(Files::EscapePath::toPathContainer(variables["data"].as<Files::EscapePathContainer>()));
    }

    std::string local = variables["data-local"].as<Files::EscapeHashString>().toStdString();
    if (!local.empty())
    {
        if (local.front() == '\"')
            local = local.substr(1, local.length() - 2);

        dataLocal.push_back(Files::PathContainer::value_type(local));
    }

    mCfgMgr.processPaths (dataDirs);
    mCfgMgr.processPaths (dataLocal, true);

    if (!dataLocal.empty())
        mLocal = dataLocal[0];
    else
    {
        QMessageBox messageBox;
        messageBox.setWindowTitle (tr ("No local data path available"));
        messageBox.setIcon (QMessageBox::Critical);
        messageBox.setStandardButtons (QMessageBox::Ok);
        messageBox.setText(tr("<br><b>OpenCS is unable to access the local data directory. This may indicate a faulty configuration or a broken install.</b>"));
        messageBox.exec();

        QApplication::exit (1);
    }

    dataDirs.insert (dataDirs.end(), dataLocal.begin(), dataLocal.end());

    //iterate the data directories and add them to the file dialog for loading
    for (Files::PathContainer::const_reverse_iterator iter = dataDirs.rbegin(); iter != dataDirs.rend(); ++iter)
    {
        QString path = QString::fromUtf8 (iter->string().c_str());
        mFileDialog.addFiles(path);
    }

    return std::make_pair (dataDirs, variables["fallback-archive"].as<Files::EscapeStringVector>().toStdStringVector());
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
    readConfig(/*quiet*/true);

    mFileDialog.showDialog (CSVDoc::ContentAction_New);
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
    readConfig(/*quiet*/true);

    mFileDialog.showDialog (CSVDoc::ContentAction_Edit);
}

void CS::Editor::openFiles (const boost::filesystem::path &savePath, const std::vector<boost::filesystem::path> &discoveredFiles)
{
    std::vector<boost::filesystem::path> files;

    if(discoveredFiles.empty())
    {
        for (const QString &path : mFileDialog.selectedFilePaths())
            files.push_back(path.toUtf8().constData());
    }
    else
    {
        files = discoveredFiles;
    }

    mDocumentManager.addDocument (files, savePath, false);

    mFileDialog.hide();
}

void CS::Editor::createNewFile (const boost::filesystem::path &savePath)
{
    std::vector<boost::filesystem::path> files;

    for (const QString &path : mFileDialog.selectedFilePaths()) {
        files.push_back(path.toUtf8().constData());
    }

    files.push_back (savePath);

    mDocumentManager.addDocument (files, savePath, true);

    mFileDialog.hide();
}

void CS::Editor::createNewGame (const boost::filesystem::path& file)
{
    std::vector<boost::filesystem::path> files;

    files.push_back (file);

    mDocumentManager.addDocument (files, file, true);

    mNewGame.hide();
}

void CS::Editor::showStartup()
{
    if(mStartup.isHidden())
        mStartup.show();
    mStartup.raise();
    mStartup.activateWindow();
}

void CS::Editor::showSettings()
{
    if (mSettings.isHidden())
        mSettings.show();

    mSettings.move (QCursor::pos());
    mSettings.raise();
    mSettings.activateWindow();
}

bool CS::Editor::makeIPCServer()
{
    try
    {
        mPid = boost::filesystem::temp_directory_path();
        mPid /= "openmw-cs.pid";
        bool pidExists = boost::filesystem::exists(mPid);

        mPidFile.open(mPid);

        mLock = boost::interprocess::file_lock(mPid.string().c_str());
        if(!mLock.try_lock())
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

        if(pidExists)
        {
            // hack to get the temp directory path
            mServer->listen("dummy");
            QString fullPath = mServer->fullServerName();
            mServer->close();
            fullPath.remove(QRegExp("dummy$"));
            fullPath += mIpcServerName;
            if(boost::filesystem::exists(fullPath.toUtf8().constData()))
            {
                // TODO: compare pid of the current process with that in the file
                Log(Debug::Info) << "Detected unclean shutdown.";
                // delete the stale file
                if(remove(fullPath.toUtf8().constData()))
                    Log(Debug::Error) << "Error: can not remove stale connection file.";
            }
        }
    }

    catch(const std::exception& e)
    {
        Log(Debug::Error) << "Error: " << e.what();
        return false;
    }

    if(mServer->listen(mIpcServerName))
    {
        connect(mServer, SIGNAL(newConnection()), this, SLOT(showStartup()));
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
        ToUTF8::Utf8Encoder encoder = ToUTF8::calculateEncoding(mEncodingName);
        fileReader.setEncoder(&encoder);
        fileReader.open(mFileToLoad.string());

        std::vector<boost::filesystem::path> discoveredFiles;

        for (std::vector<ESM::Header::MasterData>::const_iterator itemIter = fileReader.getGameFiles().begin();
            itemIter != fileReader.getGameFiles().end(); ++itemIter)
        {
            for (Files::PathContainer::const_iterator pathIter = mDataDirs.begin();
                pathIter != mDataDirs.end(); ++pathIter)
            {
                const boost::filesystem::path masterPath = *pathIter / itemIter->name;
                if (boost::filesystem::exists(masterPath))
                {
                    discoveredFiles.push_back(masterPath);
                    break;
                }
            }
        }
        discoveredFiles.push_back(mFileToLoad);

        QString extension = QString::fromStdString(mFileToLoad.extension().string()).toLower();
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

void CS::Editor::documentAdded (CSMDoc::Document *document)
{
    mViewManager->addView (document);
}

void CS::Editor::documentAboutToBeRemoved (CSMDoc::Document *document)
{
    if (mMerge.getDocument()==document)
        mMerge.cancel();
}

void CS::Editor::lastDocumentDeleted()
{
    QApplication::quit();
}

void CS::Editor::mergeDocument (CSMDoc::Document *document)
{
    mMerge.configure (document);
    mMerge.show();
    mMerge.raise();
    mMerge.activateWindow();
}
