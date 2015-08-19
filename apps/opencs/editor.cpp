#include "editor.hpp"

#include <QApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <QMessageBox>

#include <components/vfs/manager.hpp>
#include <components/vfs/registerarchives.hpp>

#include <components/nifosg/nifloader.hpp>

#include "model/doc/document.hpp"
#include "model/world/data.hpp"

#ifdef _WIN32
#include <Windows.h>
#endif

CS::Editor::Editor ()
: mUserSettings (mCfgMgr), mDocumentManager (mCfgMgr),
  mViewManager (mDocumentManager), mPid(""),
  mLock(), mIpcServerName ("org.openmw.OpenCS"), mServer(NULL), mClientSocket(NULL)
{
    std::pair<Files::PathContainer, std::vector<std::string> > config = readConfig();

    setupDataFiles (config.first);

    CSMSettings::UserSettings::instance().loadSettings ("opencs.ini");
    mSettings.setModel (CSMSettings::UserSettings::instance());

    NifOsg::Loader::setShowMarkers(true);

    mVFS.reset(new VFS::Manager(mFsStrict));

    VFS::registerArchives(mVFS.get(), Files::Collections(config.first, !mFsStrict), config.second, true);

    mDocumentManager.setVFS(mVFS.get());

    mNewGame.setLocalData (mLocal);
    mFileDialog.setLocalData (mLocal);

    connect (&mDocumentManager, SIGNAL (documentAdded (CSMDoc::Document *)),
        this, SLOT (documentAdded (CSMDoc::Document *)));
    connect (&mDocumentManager, SIGNAL (lastDocumentDeleted()),
        this, SLOT (lastDocumentDeleted()));

    connect (&mViewManager, SIGNAL (newGameRequest ()), this, SLOT (createGame ()));
    connect (&mViewManager, SIGNAL (newAddonRequest ()), this, SLOT (createAddon ()));
    connect (&mViewManager, SIGNAL (loadDocumentRequest ()), this, SLOT (loadDocument ()));
    connect (&mViewManager, SIGNAL (editSettingsRequest()), this, SLOT (showSettings ()));

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
    mPidFile.close();

    if(mServer && boost::filesystem::exists(mPid))
        static_cast<void> ( // silence coverity warning
        remove(mPid.string().c_str())); // ignore any error
}

void CS::Editor::setupDataFiles (const Files::PathContainer& dataDirs)
{
    for (Files::PathContainer::const_iterator iter = dataDirs.begin(); iter != dataDirs.end(); ++iter)
    {
        QString path = QString::fromUtf8 (iter->string().c_str());
        mFileDialog.addFiles(path);
    }
}

std::pair<Files::PathContainer, std::vector<std::string> > CS::Editor::readConfig(bool quiet)
{
    boost::program_options::variables_map variables;
    boost::program_options::options_description desc("Syntax: openmw-cs <options>\nAllowed options");

    desc.add_options()
    ("data", boost::program_options::value<Files::PathContainer>()->default_value(Files::PathContainer(), "data")->multitoken()->composing())
    ("data-local", boost::program_options::value<std::string>()->default_value(""))
    ("fs-strict", boost::program_options::value<bool>()->implicit_value(true)->default_value(false))
    ("encoding", boost::program_options::value<std::string>()->default_value("win1252"))
    ("resources", boost::program_options::value<std::string>()->default_value("resources"))
    ("fallback-archive", boost::program_options::value<std::vector<std::string> >()->
        default_value(std::vector<std::string>(), "fallback-archive")->multitoken())
    ("script-blacklist", boost::program_options::value<std::vector<std::string> >()->default_value(std::vector<std::string>(), "")
        ->multitoken(), "exclude specified script from the verifier (if the use of the blacklist is enabled)")
    ("script-blacklist-use", boost::program_options::value<bool>()->implicit_value(true)
        ->default_value(true), "enable script blacklisting");

    boost::program_options::notify(variables);

    mCfgMgr.readConfiguration(variables, desc, quiet);

    mDocumentManager.setEncoding (
        ToUTF8::calculateEncoding (variables["encoding"].as<std::string>()));

    mDocumentManager.setResourceDir (mResources = variables["resources"].as<std::string>());

    if (variables["script-blacklist-use"].as<bool>())
        mDocumentManager.setBlacklistedScripts (
            variables["script-blacklist"].as<std::vector<std::string> >());

    mFsStrict = variables["fs-strict"].as<bool>();

    Files::PathContainer dataDirs, dataLocal;
    if (!variables["data"].empty()) {
        dataDirs = Files::PathContainer(variables["data"].as<Files::PathContainer>());
    }

    std::string local = variables["data-local"].as<std::string>();
    if (!local.empty()) {
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
    for (Files::PathContainer::const_iterator iter = dataDirs.begin(); iter != dataDirs.end(); ++iter)
    {
        QString path = QString::fromUtf8 (iter->string().c_str());
        mFileDialog.addFiles(path);
    }

    return std::make_pair (dataDirs, variables["fallback-archive"].as<std::vector<std::string> >());
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
    std::pair<Files::PathContainer, std::vector<std::string> > config = readConfig(/*quiet*/true);
    setupDataFiles (config.first);

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
    std::pair<Files::PathContainer, std::vector<std::string> > config = readConfig(/*quiet*/true);
    setupDataFiles (config.first);

    mFileDialog.showDialog (CSVDoc::ContentAction_Edit);
}

void CS::Editor::openFiles (const boost::filesystem::path &savePath)
{
    std::vector<boost::filesystem::path> files;

    foreach (const QString &path, mFileDialog.selectedFilePaths())
        files.push_back(path.toUtf8().constData());

    mDocumentManager.addDocument (files, savePath, false);

    mFileDialog.hide();
}

void CS::Editor::createNewFile (const boost::filesystem::path &savePath)
{
    std::vector<boost::filesystem::path> files;

    foreach (const QString &path, mFileDialog.selectedFilePaths()) {
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
            std::cerr << "OpenCS already running."  << std::endl;
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
                std::cout << "Detected unclean shutdown." << std::endl;
                // delete the stale file
                if(remove(fullPath.toUtf8().constData()))
                    std::cerr << "ERROR removing stale connection file" << std::endl;
            }
        }
    }

    catch(const std::exception& e)
    {
        std::cerr << "ERROR " << e.what() << std::endl;
        return false;
    }

    if(mServer->listen(mIpcServerName))
    {
        connect(mServer, SIGNAL(newConnection()), this, SLOT(showStartup()));
        return true;
    }

    mServer->close();
    mServer = NULL;
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

    mStartup.show();

    QApplication::setQuitOnLastWindowClosed (true);

    return QApplication::exec();
}

void CS::Editor::documentAdded (CSMDoc::Document *document)
{
    mViewManager.addView (document);
}

void CS::Editor::lastDocumentDeleted()
{
    QApplication::quit();
}
