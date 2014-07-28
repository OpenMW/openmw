
#include "editor.hpp"

#include <QApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <QMessageBox>

#include <OgreRoot.h>
#include <OgreRenderWindow.h>

#include <extern/shiny/Main/Factory.hpp>
#include <extern/shiny/Platforms/Ogre/OgrePlatform.hpp>

#include <components/ogreinit/ogreinit.hpp>

#include <components/bsa/resources.hpp>

#include "model/doc/document.hpp"
#include "model/world/data.hpp"

CS::Editor::Editor (OgreInit::OgreInit& ogreInit)
: mUserSettings (mCfgMgr), mDocumentManager (mCfgMgr), mViewManager (mDocumentManager),
  mIpcServerName ("org.openmw.OpenCS")
{
    std::pair<Files::PathContainer, std::vector<std::string> > config = readConfig();

    setupDataFiles (config.first);

    CSMSettings::UserSettings::instance().loadSettings ("opencs.ini");
    mSettings.setModel (CSMSettings::UserSettings::instance());

    ogreInit.init ((mCfgMgr.getUserConfigPath() / "opencsOgre.log").string());

    Bsa::registerResources (Files::Collections (config.first, !mFsStrict), config.second, true,
        mFsStrict);

    mDocumentManager.listResources();

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

    connect (&mNewGame, SIGNAL (createRequest (const boost::filesystem::path&)),
             this, SLOT (createNewGame (const boost::filesystem::path&)));
}

void CS::Editor::setupDataFiles (const Files::PathContainer& dataDirs)
{
    for (Files::PathContainer::const_iterator iter = dataDirs.begin(); iter != dataDirs.end(); ++iter)
    {
        QString path = QString::fromUtf8 (iter->string().c_str());
        mFileDialog.addFiles(path);
    }
}

std::pair<Files::PathContainer, std::vector<std::string> > CS::Editor::readConfig()
{
    boost::program_options::variables_map variables;
    boost::program_options::options_description desc("Syntax: opencs <options>\nAllowed options");

    desc.add_options()
    ("data", boost::program_options::value<Files::PathContainer>()->default_value(Files::PathContainer(), "data")->multitoken())
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

    mCfgMgr.readConfiguration(variables, desc);

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

void CS::Editor::createAddon()
{
    mStartup.hide();
    mFileDialog.showDialog (CSVDoc::ContentAction_New);
}

void CS::Editor::loadDocument()
{
    mStartup.hide();
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

    files.push_back(mFileDialog.filename().toUtf8().constData());

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

    mSettings.raise();
    mSettings.activateWindow();
}

bool CS::Editor::makeIPCServer()
{
    mServer = new QLocalServer(this);

    if(mServer->listen(mIpcServerName))
    {
        connect(mServer, SIGNAL(newConnection()), this, SLOT(showStartup()));
        return true;
    }

    mServer->close();
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

std::auto_ptr<sh::Factory> CS::Editor::setupGraphics()
{
    // TODO: setting
    Ogre::Root::getSingleton().setRenderSystem(Ogre::Root::getSingleton().getRenderSystemByName("OpenGL Rendering Subsystem"));

    Ogre::Root::getSingleton().initialise(false);

    // Create a hidden background window to keep resources
    Ogre::NameValuePairList params;
    params.insert(std::make_pair("title", ""));
    params.insert(std::make_pair("FSAA", "0"));
    params.insert(std::make_pair("vsync", "false"));
    params.insert(std::make_pair("hidden", "true"));
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
    params.insert(std::make_pair("macAPI", "cocoa"));
#endif
    Ogre::RenderWindow* hiddenWindow = Ogre::Root::getSingleton().createRenderWindow("InactiveHidden", 1, 1, false, &params);
    hiddenWindow->setActive(false);

    sh::OgrePlatform* platform =
        new sh::OgrePlatform ("General", (mResources / "materials").string());

    if (!boost::filesystem::exists (mCfgMgr.getCachePath()))
        boost::filesystem::create_directories (mCfgMgr.getCachePath());

    platform->setCacheFolder (mCfgMgr.getCachePath().string());

    std::auto_ptr<sh::Factory> factory (new sh::Factory (platform));

    factory->setCurrentLanguage (sh::Language_GLSL); /// \todo make this configurable
    factory->setWriteSourceCache (true);
    factory->setReadSourceCache (true);
    factory->setReadMicrocodeCache (true);
    factory->setWriteMicrocodeCache (true);

    factory->loadAllFiles();

    sh::Factory::getInstance().setGlobalSetting ("fog", "true");

    sh::Factory::getInstance().setGlobalSetting ("shadows", "false");
    sh::Factory::getInstance().setGlobalSetting ("shadows_pssm", "false");

    sh::Factory::getInstance ().setGlobalSetting ("render_refraction", "false");

    sh::Factory::getInstance ().setGlobalSetting ("viewproj_fix", "false");

    sh::Factory::getInstance ().setGlobalSetting ("num_lights", "8");

    /// \todo add more configurable shiny settings

    return factory;
}

void CS::Editor::documentAdded (CSMDoc::Document *document)
{
    mViewManager.addView (document);
}

void CS::Editor::lastDocumentDeleted()
{
    exit (0);
}
