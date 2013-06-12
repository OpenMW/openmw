
#include "editor.hpp"

#include <QtGui/QApplication>

#include "model/doc/document.hpp"
#include "model/world/data.hpp"

CS::Editor::Editor() : mViewManager (mDocumentManager)
{
    connect (&mViewManager, SIGNAL (newDocumentRequest ()), this, SLOT (createDocument ()));
    connect (&mViewManager, SIGNAL (loadDocumentRequest ()), this, SLOT (loadDocument ()));

    connect (&mStartup, SIGNAL (createDocument()), this, SLOT (createDocument ()));
    connect (&mStartup, SIGNAL (loadDocument()), this, SLOT (loadDocument ()));

    connect (&mFileDialog, SIGNAL(openFiles()), this, SLOT(openFiles()));
    connect (&mFileDialog, SIGNAL(createNewFile()), this, SLOT(createNewFile()));

    setupDataFiles();
}

void CS::Editor::setupDataFiles()
{
    boost::program_options::variables_map variables;
    boost::program_options::options_description desc;

    desc.add_options()
    ("data", boost::program_options::value<Files::PathContainer>()->default_value(Files::PathContainer(), "data")->multitoken())
    ("data-local", boost::program_options::value<std::string>()->default_value(""))
    ("fs-strict", boost::program_options::value<bool>()->implicit_value(true)->default_value(false))
    ("encoding", boost::program_options::value<std::string>()->default_value("win1252"));

    boost::program_options::notify(variables);

    mCfgMgr.readConfiguration(variables, desc);

    Files::PathContainer mDataDirs, mDataLocal;
    if (!variables["data"].empty()) {
        mDataDirs = Files::PathContainer(variables["data"].as<Files::PathContainer>());
    }

    std::string local = variables["data-local"].as<std::string>();
    if (!local.empty()) {
        mDataLocal.push_back(Files::PathContainer::value_type(local));
    }

    mCfgMgr.processPaths(mDataDirs);
    mCfgMgr.processPaths(mDataLocal);

    // Set the charset for reading the esm/esp files
    QString encoding = QString::fromStdString(variables["encoding"].as<std::string>());
    mFileDialog.setEncoding(encoding);

    Files::PathContainer dataDirs;
    dataDirs.insert(dataDirs.end(), mDataDirs.begin(), mDataDirs.end());
    dataDirs.insert(dataDirs.end(), mDataLocal.begin(), mDataLocal.end());

    for (Files::PathContainer::const_iterator iter = dataDirs.begin(); iter != dataDirs.end(); ++iter)
    {
        QString path = QString::fromStdString(iter->string());
        mFileDialog.addFiles(path);
    }
}

void CS::Editor::createDocument()
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

    CSMDoc::Document *document = mDocumentManager.addDocument(files, false);

    mViewManager.addView (document);
    mFileDialog.hide();
}

void CS::Editor::createNewFile()
{
    std::vector<boost::filesystem::path> files;
    QStringList paths = mFileDialog.checkedItemsPaths();

    foreach (const QString &path, paths) {
        files.push_back(path.toStdString());
    }

    files.push_back(mFileDialog.fileName().toStdString());

    CSMDoc::Document *document = mDocumentManager.addDocument (files, true);

    mViewManager.addView (document);
    mFileDialog.hide();
}

int CS::Editor::run()
{
    mStartup.show();

    QApplication::setQuitOnLastWindowClosed (true);

    return QApplication::exec();
}
