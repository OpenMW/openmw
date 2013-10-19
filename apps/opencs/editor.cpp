
#include "editor.hpp"

#include <QApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <QMessageBox>

#include "model/doc/document.hpp"
#include "model/world/data.hpp"
#include <iostream>


CS::Editor::Editor()
    : mDocumentManager (mCfgMgr), mViewManager (mDocumentManager)
{
    mIpcServerName = "org.openmw.OpenCS";

    setupDataFiles();

    mNewGame.setLocalData (mLocal);
    mFileDialog.setLocalData (mLocal);

    connect (&mViewManager, SIGNAL (newGameRequest ()), this, SLOT (createGame ()));
    connect (&mViewManager, SIGNAL (newAddonRequest ()), this, SLOT (createAddon ()));
    connect (&mViewManager, SIGNAL (loadDocumentRequest ()), this, SLOT (loadDocument ()));
    connect (&mViewManager, SIGNAL (editSettingsRequest()), this, SLOT (showSettings ()));

    connect (&mStartup, SIGNAL (createGame()), this, SLOT (createGame ()));
    connect (&mStartup, SIGNAL (createAddon()), this, SLOT (createAddon ()));
    connect (&mStartup, SIGNAL (loadDocument()), this, SLOT (loadDocument ()));
    connect (&mStartup, SIGNAL (editConfig()), this, SLOT (showSettings ()));

    connect (&mFileDialog, SIGNAL(openFiles()), this, SLOT(openFiles()));
    connect (&mFileDialog, SIGNAL(createNewFile (const boost::filesystem::path&)),
             this, SLOT(createNewFile (const boost::filesystem::path&)));

    connect (&mNewGame, SIGNAL (createRequest (const boost::filesystem::path&)),
             this, SLOT (createNewGame (const boost::filesystem::path&)));
}

void CS::Editor::setupDataFiles()
{
    boost::program_options::variables_map variables;
    boost::program_options::options_description desc("Syntax: opencs <options>\nAllowed options");

    desc.add_options()
    ("data", boost::program_options::value<Files::PathContainer>()->default_value(Files::PathContainer(), "data")->multitoken())
    ("data-local", boost::program_options::value<std::string>()->default_value(""))
    ("fs-strict", boost::program_options::value<bool>()->implicit_value(true)->default_value(false))
    ("encoding", boost::program_options::value<std::string>()->default_value("win1252"))
    ("resources", boost::program_options::value<std::string>()->default_value("resources"));

    boost::program_options::notify(variables);

    mCfgMgr.readConfiguration(variables, desc);

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
        return;
    }

    // Set the charset for reading the esm/esp files
    // QString encoding = QString::fromStdString(variables["encoding"].as<std::string>());
    //mFileDialog.setEncoding(encoding);

    dataDirs.insert (dataDirs.end(), dataLocal.begin(), dataLocal.end());

//     Adding Resources directory. First check if there is a file defaultfilters in the user path.
    mDocumentManager.setResourceDir(variables["resources"].as<std::string>());

    for (Files::PathContainer::const_iterator iter = dataDirs.begin(); iter != dataDirs.end(); ++iter)
    {
        QString path = QString::fromStdString(iter->string());
        mFileDialog.addFiles(path);
    }

    //load the settings into the userSettings instance.
    const QString settingFileName = "opencs.cfg";
    CSMSettings::UserSettings::instance().loadSettings(settingFileName);
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

    mFileDialog.newFile();
}

void CS::Editor::loadDocument()
{
    mStartup.hide();

    mFileDialog.openFile();
}

void CS::Editor::openFiles()
{
    std::vector<boost::filesystem::path> files;
    QStringList paths = mFileDialog.checkedItemsPaths();

    foreach (const QString &path, paths) {
        files.push_back(path.toStdString());
    }

    /// \todo Get the save path from the file dialogue

    CSMDoc::Document *document = mDocumentManager.addDocument (files, *files.rbegin(), false);

    mViewManager.addView (document);
    mFileDialog.hide();
}

void CS::Editor::createNewFile (const boost::filesystem::path& savePath)
{
    std::vector<boost::filesystem::path> files;
    QStringList paths = mFileDialog.checkedItemsPaths();

    foreach (const QString &path, paths) {
        files.push_back(path.toStdString());
    }

    files.push_back(mFileDialog.fileName().toStdString());

    CSMDoc::Document *document = mDocumentManager.addDocument (files, savePath, true);

    mViewManager.addView (document);
    mFileDialog.hide();
}

void CS::Editor::createNewGame (const boost::filesystem::path& file)
{
    std::vector<boost::filesystem::path> files;

    files.push_back (file);

    CSMDoc::Document *document = mDocumentManager.addDocument (files, file, true);

    mViewManager.addView (document);

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
