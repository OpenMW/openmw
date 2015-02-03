
#include "editor.hpp"

#include <openengine/bullet/BulletShapeLoader.h>

#include <QApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <QMessageBox>

#include <OgreRoot.h>
#include <OgreRenderWindow.h>

#include <extern/shiny/Main/Factory.hpp>
#include <extern/shiny/Platforms/Ogre/OgrePlatform.hpp>

#include <components/ogreinit/ogreinit.hpp>
#include <components/nifogre/ogrenifloader.hpp>
#include <components/bsa/resources.hpp>

#include "model/doc/document.hpp"
#include "model/world/data.hpp"

CS::Editor::Editor (OgreInit::OgreInit& ogreInit)
: mUserSettings (mCfgMgr), mOverlaySystem (0), mDocumentManager (mCfgMgr),
  mViewManager (mDocumentManager),
  mIpcServerName ("org.openmw.OpenCS"), mServer(NULL), mClientSocket(NULL), mPid(""), mLock()
{
    std::pair<Files::PathContainer, std::vector<std::string> > config = readConfig();

    setupDataFiles (config.first);

    CSMSettings::UserSettings::instance().loadSettings ("opencs.ini");
    mSettings.setModel (CSMSettings::UserSettings::instance());

    ogreInit.init ((mCfgMgr.getUserConfigPath() / "opencsOgre.log").string());

    NifOgre::Loader::setShowMarkers(true);

    mOverlaySystem.reset (new CSVRender::OverlaySystem);

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

CS::Editor::~Editor ()
{
    mPidFile.close();

    if(mServer && boost::filesystem::exists(mPid))
        static_cast<void> ( // silence coverity warning
        remove(mPid.string().c_str())); // ignore any error

    // cleanup global resources used by OEngine
    delete OEngine::Physic::BulletShapeManager::getSingletonPtr();
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
            if(boost::filesystem::exists(fullPath.toStdString().c_str()))
            {
                // TODO: compare pid of the current process with that in the file
                std::cout << "Detected unclean shutdown." << std::endl;
                // delete the stale file
                if(remove(fullPath.toStdString().c_str()))
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

std::auto_ptr<sh::Factory> CS::Editor::setupGraphics()
{
    std::string renderer =
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
        "Direct3D9 Rendering Subsystem";
#else
        "OpenGL Rendering Subsystem";
#endif
    std::string renderSystem = mUserSettings.setting("Video/render system", renderer.c_str()).toStdString();

    Ogre::Root::getSingleton().setRenderSystem(Ogre::Root::getSingleton().getRenderSystemByName(renderSystem));

    // Initialise Ogre::OverlaySystem after Ogre::Root but before initialisation
    mOverlaySystem.get();

    Ogre::Root::getSingleton().initialise(false);

    // Create a hidden background window to keep resources
    Ogre::NameValuePairList params;
    params.insert(std::make_pair("title", ""));

    std::string antialiasing = mUserSettings.settingValue("Video/antialiasing").toStdString();
    if(antialiasing == "MSAA 16")     antialiasing = "16";
    else if(antialiasing == "MSAA 8") antialiasing = "8";
    else if(antialiasing == "MSAA 4") antialiasing = "4";
    else if(antialiasing == "MSAA 2") antialiasing = "2";
    else                              antialiasing = "0";
    params.insert(std::make_pair("FSAA", antialiasing));

    params.insert(std::make_pair("vsync", "false"));
    params.insert(std::make_pair("hidden", "true"));
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
    params.insert(std::make_pair("macAPI", "cocoa"));
#endif
    // NOTE: fullscreen mode not supported (doesn't really make sense for opencs)
    Ogre::RenderWindow* hiddenWindow = Ogre::Root::getSingleton().createRenderWindow("InactiveHidden", 1, 1, false, &params);
    hiddenWindow->setActive(false);

    sh::OgrePlatform* platform =
        new sh::OgrePlatform ("General", (mResources / "materials").string());

    // for font used in overlays
    Ogre::Root::getSingleton().addResourceLocation ((mResources / "mygui").string(),
            "FileSystem", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, true);

    if (!boost::filesystem::exists (mCfgMgr.getCachePath()))
        boost::filesystem::create_directories (mCfgMgr.getCachePath());

    platform->setCacheFolder (mCfgMgr.getCachePath().string());

    std::auto_ptr<sh::Factory> factory (new sh::Factory (platform));

    QString shLang = mUserSettings.settingValue("General/shader mode");
    QString rend = renderSystem.c_str();
    bool openGL = rend.contains(QRegExp("^OpenGL", Qt::CaseInsensitive));
    bool glES = rend.contains(QRegExp("^OpenGL ES", Qt::CaseInsensitive));

    // force shader language based on render system
    if(shLang == ""
            || (openGL && shLang == "hlsl")
            || (!openGL && shLang == "glsl")
            || (glES && shLang != "glsles"))
    {
        shLang = openGL ? (glES ? "glsles" : "glsl") : "hlsl";
        //no group means "General" group in the "ini" file standard
        mUserSettings.setDefinitions("shader mode", (QStringList() << shLang));
    }
    enum sh::Language lang;
    if(shLang == "glsl")        lang = sh::Language_GLSL;
    else if(shLang == "glsles") lang = sh::Language_GLSLES;
    else if(shLang == "hlsl")   lang = sh::Language_HLSL;
    else                        lang = sh::Language_CG;

    factory->setCurrentLanguage (lang);
    factory->setWriteSourceCache (true);
    factory->setReadSourceCache (true);
    factory->setReadMicrocodeCache (true);
    factory->setWriteMicrocodeCache (true);

    factory->loadAllFiles();

    bool shaders = mUserSettings.setting("3d-render/shaders", QString("true")) == "true" ? true : false;
    sh::Factory::getInstance ().setShadersEnabled (shaders);

    std::string fog = mUserSettings.setting("Shader/fog", QString("true")).toStdString();
    sh::Factory::getInstance().setGlobalSetting ("fog", fog);


    std::string shadows = mUserSettings.setting("Shader/shadows", QString("false")).toStdString();
    sh::Factory::getInstance().setGlobalSetting ("shadows", shadows);

    std::string shadows_pssm = mUserSettings.setting("Shader/shadows_pssm", QString("false")).toStdString();
    sh::Factory::getInstance().setGlobalSetting ("shadows_pssm", shadows_pssm);

    std::string render_refraction = mUserSettings.setting("Shader/render_refraction", QString("false")).toStdString();
    sh::Factory::getInstance ().setGlobalSetting ("render_refraction", render_refraction);

    // internal setting - may be switched on or off by the use of shader configurations
    sh::Factory::getInstance ().setGlobalSetting ("viewproj_fix", "false");

    std::string num_lights = mUserSettings.setting("3d-render-adv/num_lights", QString("8")).toStdString();
    sh::Factory::getInstance ().setGlobalSetting ("num_lights", num_lights);

    /// \todo add more configurable shiny settings

    return factory;
}

void CS::Editor::documentAdded (CSMDoc::Document *document)
{
    mViewManager.addView (document);
}

void CS::Editor::lastDocumentDeleted()
{
    QApplication::quit();
}
