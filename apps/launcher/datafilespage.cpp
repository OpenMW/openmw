#include <QtGui>

#include <components/esm/esmreader.hpp>
#include <components/files/configurationmanager.hpp>
#include <components/fileorderlist/datafileslist.hpp>
#include <components/fileorderlist/utils/lineedit.hpp>
#include <components/fileorderlist/utils/naturalsort.hpp>
#include <components/fileorderlist/utils/filedialog.hpp>

////#include "model/datafilesmodel.hpp"
////#include "model/esm/esmfile.hpp"

#include "utils/profilescombobox.hpp"
////#include "utils/filedialog.hpp"
////#include "utils/naturalsort.hpp"
#include "utils/textinputdialog.hpp"

#include "datafilespage.hpp"

#include <boost/version.hpp>
/**
 * Workaround for problems with whitespaces in paths in older versions of Boost library
 */
#if (BOOST_VERSION <= 104600)
namespace boost
{

    template<>
    inline boost::filesystem::path lexical_cast<boost::filesystem::path, std::string>(const std::string& arg)
    {
        return boost::filesystem::path(arg);
    }

} /* namespace boost */
#endif /* (BOOST_VERSION <= 104600) */

using namespace ESM;
using namespace std;

DataFilesPage::DataFilesPage(Files::ConfigurationManager &cfg, QWidget *parent)
    : QWidget(parent)
    , mCfgMgr(cfg)
{
    mDataFilesList = new DataFilesList(mCfgMgr, this);

    // Bottom part with profile options
    QLabel *profileLabel = new QLabel(tr("Current Profile: "), this);

    mProfilesComboBox = new ProfilesComboBox(this);
    mProfilesComboBox->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
    mProfilesComboBox->setInsertPolicy(QComboBox::NoInsert);
    mProfilesComboBox->setDuplicatesEnabled(false);
    mProfilesComboBox->setEditEnabled(false);

    mProfileToolBar = new QToolBar(this);
    mProfileToolBar->setMovable(false);
    mProfileToolBar->setIconSize(QSize(16, 16));

    mProfileToolBar->addWidget(profileLabel);
    mProfileToolBar->addWidget(mProfilesComboBox);

    QVBoxLayout *pageLayout = new QVBoxLayout(this);

    pageLayout->addWidget(mDataFilesList);
    pageLayout->addWidget(mProfileToolBar);

    // Create a dialog for the new profile name input
    mNewProfileDialog = new TextInputDialog(tr("New Profile"), tr("Profile name:"), this);

    connect(mNewProfileDialog->lineEdit(), SIGNAL(textChanged(QString)), this, SLOT(updateOkButton(QString)));
    
    connect(mProfilesComboBox, SIGNAL(profileRenamed(QString,QString)), this, SLOT(profileRenamed(QString,QString)));
    connect(mProfilesComboBox, SIGNAL(profileChanged(QString,QString)), this, SLOT(profileChanged(QString,QString)));

    createActions();
    setupConfig();
}

void DataFilesPage::createActions()
{
    // Refresh the plugins
    QAction *refreshAction = new QAction(tr("Refresh"), this);
    refreshAction->setShortcut(QKeySequence(tr("F5")));
    connect(refreshAction, SIGNAL(triggered()), this, SLOT(refresh()));

    // Profile actions
    mNewProfileAction = new QAction(QIcon::fromTheme("document-new"), tr("&New Profile"), this);
    mNewProfileAction->setToolTip(tr("New Profile"));
    mNewProfileAction->setShortcut(QKeySequence(tr("Ctrl+N")));
    connect(mNewProfileAction, SIGNAL(triggered()), this, SLOT(newProfile()));

    mDeleteProfileAction = new QAction(QIcon::fromTheme("edit-delete"), tr("Delete Profile"), this);
    mDeleteProfileAction->setToolTip(tr("Delete Profile"));
    mDeleteProfileAction->setShortcut(QKeySequence(tr("Delete")));
    connect(mDeleteProfileAction, SIGNAL(triggered()), this, SLOT(deleteProfile()));

    // Add the newly created actions to the toolbar
    mProfileToolBar->addSeparator();
    mProfileToolBar->addAction(mNewProfileAction);
    mProfileToolBar->addAction(mDeleteProfileAction);
}

void DataFilesPage::setupConfig()
{
    // Open our config file
    QString config = QString::fromStdString((mCfgMgr.getUserPath() / "launcher.cfg").string());
    mLauncherConfig = new QSettings(config, QSettings::IniFormat);

    // Make sure we have no groups open
    while (!mLauncherConfig->group().isEmpty()) {
        mLauncherConfig->endGroup();
    }

    mLauncherConfig->beginGroup("Profiles");
    QStringList profiles = mLauncherConfig->childGroups();

    // Add the profiles to the combobox
    foreach (const QString &profile, profiles) {

        if (profile.contains(QRegExp("[^a-zA-Z0-9_]")))
            continue; // Profile name contains garbage


         qDebug() << "adding " << profile;
         mProfilesComboBox->addItem(profile);
    }

    // Add a default profile
    if (mProfilesComboBox->findText(QString("Default")) == -1) {
         mProfilesComboBox->addItem(QString("Default"));
    }

    QString currentProfile = mLauncherConfig->value("CurrentProfile").toString();

    if (currentProfile.isEmpty()) {
        // No current profile selected
        currentProfile = "Default";
    }

    const int currentIndex = mProfilesComboBox->findText(currentProfile);
    if (currentIndex != -1) {
        // Profile is found
        mProfilesComboBox->setCurrentIndex(currentIndex);
    }

    mLauncherConfig->endGroup();
}


void DataFilesPage::readConfig()
{
    QString profile = mProfilesComboBox->currentText();
    
    // Make sure we have no groups open
    while (!mLauncherConfig->group().isEmpty()) {
        mLauncherConfig->endGroup();
    }

    mLauncherConfig->beginGroup("Profiles");
    mLauncherConfig->beginGroup(profile);

    QStringList childKeys = mLauncherConfig->childKeys();
    QStringList plugins;

    // Sort the child keys numerical instead of alphabetically
    // i.e. Plugin1, Plugin2 instead of Plugin1, Plugin10
    qSort(childKeys.begin(), childKeys.end(), naturalSortLessThanCI);

    foreach (const QString &key, childKeys) {
        const QString keyValue = mLauncherConfig->value(key).toString();
        
        mDataFilesList->setCheckState(keyValue, Qt::Checked);
    }

    qDebug() << plugins;
}

bool DataFilesPage::showDataFilesWarning()
{

    QMessageBox msgBox;
    msgBox.setWindowTitle("Error detecting Morrowind installation");
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setStandardButtons(QMessageBox::Cancel);
    msgBox.setText(tr("<br><b>Could not find the Data Files location</b><br><br> \
                      The directory containing the data files was not found.<br><br> \
                      Press \"Browse...\" to specify the location manually.<br>"));

    QAbstractButton *dirSelectButton =
            msgBox.addButton(tr("B&rowse..."), QMessageBox::ActionRole);

    msgBox.exec();

    if (msgBox.clickedButton() == dirSelectButton) {

        // Show a custom dir selection dialog which only accepts valid dirs
        QString selectedDir = FileDialog::getExistingDirectory(
                    this, tr("Select Data Files Directory"),
                    QDir::currentPath(),
                    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

        // Add the user selected data directory
        if (!selectedDir.isEmpty()) {
            mDataDirs.push_back(Files::PathContainer::value_type(selectedDir.toStdString()));
            mCfgMgr.processPaths(mDataDirs);
        } else {
            // Cancel from within the dir selection dialog
            return false;
        }

    } else {
        // Cancel
        return false;
    }

    return true;
}

bool DataFilesPage::setupDataFiles()
{
    // We use the Configuration Manager to retrieve the configuration values
    boost::program_options::variables_map variables;
    boost::program_options::options_description desc;
    
    desc.add_options()
    ("data", boost::program_options::value<Files::PathContainer>()->default_value(Files::PathContainer(), "data")->multitoken())
    ("data-local", boost::program_options::value<std::string>()->default_value(""))
    ("fs-strict", boost::program_options::value<bool>()->implicit_value(true)->default_value(false))
    ("encoding", boost::program_options::value<std::string>()->default_value("win1252"));
    
    boost::program_options::notify(variables);
    
    mCfgMgr.readConfiguration(variables, desc);
    
    if (variables["data"].empty()) {
        if (!showDataFilesWarning())
            return false;
    } else {
        mDataDirs = Files::PathContainer(variables["data"].as<Files::PathContainer>());
    }
    
    std::string local = variables["data-local"].as<std::string>();
    if (!local.empty()) {
        mDataLocal.push_back(Files::PathContainer::value_type(local));
    }
    
    mCfgMgr.processPaths(mDataDirs);
    mCfgMgr.processPaths(mDataLocal);
    
    // Second chance to display the warning, the data= entries are invalid
    while (mDataDirs.empty()) {
        if (!showDataFilesWarning())
            return false;
    }
    
    // Set the charset for reading the esm/esp files
    QString encoding = QString::fromStdString(variables["encoding"].as<std::string>());
    
    Files::PathContainer paths;
    paths.insert(paths.end(), mDataDirs.begin(), mDataDirs.end());
    paths.insert(paths.end(), mDataLocal.begin(), mDataLocal.end());
    mDataFilesList->setupDataFiles(paths, encoding);
    readConfig();
    return true;
}

void DataFilesPage::writeConfig(QString profile)
{
    QString pathStr = QString::fromStdString(mCfgMgr.getUserPath().string());
    QDir userPath(pathStr);

    if (!userPath.exists()) {
        if (!userPath.mkpath(pathStr)) {
            QMessageBox msgBox;
            msgBox.setWindowTitle("Error creating OpenMW configuration directory");
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setText(tr("<br><b>Could not create %0</b><br><br> \
                              Please make sure you have the right permissions and try again.<br>").arg(pathStr));
            msgBox.exec();

            qApp->quit();
            return;
        }
    }
    // Open the OpenMW config as a QFile
    QFile file(pathStr.append("openmw.cfg"));

    if (!file.open(QIODevice::ReadWrite | QIODevice::Text)) {
        // File cannot be opened or created
        QMessageBox msgBox;
        msgBox.setWindowTitle("Error writing OpenMW configuration file");
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setText(tr("<br><b>Could not open or create %0</b><br><br> \
                          Please make sure you have the right permissions and try again.<br>").arg(file.fileName()));
        msgBox.exec();

        qApp->quit();
        return;
    }

    QTextStream in(&file);
    QByteArray buffer;

    // Remove all previous entries from config
    while (!in.atEnd()) {
        QString line = in.readLine();
        if (!line.startsWith("master") &&
            !line.startsWith("plugin") &&
            !line.startsWith("data") &&
            !line.startsWith("data-local"))
        {
            buffer += line += "\n";
        }
    }

    file.close();

    // Now we write back the other config entries
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QMessageBox msgBox;
        msgBox.setWindowTitle("Error writing OpenMW configuration file");
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setText(tr("<br><b>Could not write to %0</b><br><br> \
                          Please make sure you have the right permissions and try again.<br>").arg(file.fileName()));
        msgBox.exec();

        qApp->quit();
        return;
    }

    if (!buffer.isEmpty()) {
        file.write(buffer);
    }

    QTextStream gameConfig(&file);

    // First write the list of data dirs
    mCfgMgr.processPaths(mDataDirs);
    mCfgMgr.processPaths(mDataLocal);

    QString path;

    // data= directories
    for (Files::PathContainer::iterator it = mDataDirs.begin(); it != mDataDirs.end(); ++it) {
        path = QString::fromStdString(it->string());
        path.remove(QChar('\"'));

        // Make sure the string is quoted when it contains spaces
        if (path.contains(" ")) {
            gameConfig << "data=\"" << path << "\"" << endl;
        } else {
            gameConfig << "data=" << path << endl;
        }
    }

    // data-local directory
    if (!mDataLocal.empty()) {
        path = QString::fromStdString(mDataLocal.front().string());
        path.remove(QChar('\"'));

        if (path.contains(" ")) {
            gameConfig << "data-local=\"" << path << "\"" << endl;
        } else {
            gameConfig << "data-local=" << path << endl;
        }
    }


    if (profile.isEmpty())
        profile = mProfilesComboBox->currentText();

    if (profile.isEmpty())
        return;

    // Make sure we have no groups open
    while (!mLauncherConfig->group().isEmpty()) {
        mLauncherConfig->endGroup();
    }

    mLauncherConfig->beginGroup("Profiles");
    mLauncherConfig->setValue("CurrentProfile", profile);

    // Open the profile-name subgroup
    mLauncherConfig->beginGroup(profile);
    mLauncherConfig->remove(""); // Clear the subgroup

    // Now write the masters to the configs
    const QStringList checkedFiles = mDataFilesList->checkedFiles();
    for(int i=0; i < checkedFiles.size(); i++)
    {
        if (checkedFiles.at(i).lastIndexOf("esm") != -1)
        {
            mLauncherConfig->setValue(QString("Master%0").arg(i), checkedFiles.at(i));
            gameConfig << "master=" << checkedFiles.at(i) << endl;
        }
        else
        {
            mLauncherConfig->setValue(QString("Plugin%1").arg(i), checkedFiles.at(i));
            gameConfig << "plugin=" << checkedFiles.at(i) << endl;
        }
    }

    file.close();
    mLauncherConfig->endGroup();
    mLauncherConfig->endGroup();
    mLauncherConfig->sync();
}


void DataFilesPage::newProfile()
{
    if (mNewProfileDialog->exec() == QDialog::Accepted) {

        const QString text = mNewProfileDialog->lineEdit()->text();
        mProfilesComboBox->addItem(text);

        // Copy the currently checked items to cfg
        writeConfig(text);
        mLauncherConfig->sync();

        mProfilesComboBox->setCurrentIndex(mProfilesComboBox->findText(text));
    }
}

void DataFilesPage::updateOkButton(const QString &text)
{
    if (text.isEmpty()) {
         mNewProfileDialog->setOkButtonEnabled(false);
         return;
    }

    (mProfilesComboBox->findText(text) == -1)
            ? mNewProfileDialog->setOkButtonEnabled(true)
            : mNewProfileDialog->setOkButtonEnabled(false);
}

void DataFilesPage::deleteProfile()
{
    QString profile = mProfilesComboBox->currentText();

    if (profile.isEmpty())
        return;

    QMessageBox msgBox(this);
    msgBox.setWindowTitle(tr("Delete Profile"));
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setStandardButtons(QMessageBox::Cancel);
    msgBox.setText(tr("Are you sure you want to delete <b>%0</b>?").arg(profile));

    QAbstractButton *deleteButton =
    msgBox.addButton(tr("Delete"), QMessageBox::ActionRole);

    msgBox.exec();

    if (msgBox.clickedButton() == deleteButton) {
       // Make sure we have no groups open
        while (!mLauncherConfig->group().isEmpty()) {
            mLauncherConfig->endGroup();
        }

        mLauncherConfig->beginGroup("Profiles");

        // Open the profile-name subgroup
        mLauncherConfig->beginGroup(profile);
        mLauncherConfig->remove(""); // Clear the subgroup
        mLauncherConfig->endGroup();
        mLauncherConfig->endGroup();

        // Remove the profile from the combobox
        mProfilesComboBox->removeItem(mProfilesComboBox->findText(profile));
    }
}

void DataFilesPage::profileChanged(const QString &previous, const QString &current)
{
    qDebug() << "Profile is changed from: " << previous << " to " << current;
    // Prevent the deletion of the default profile
    if (current == QLatin1String("Default")) {
        mDeleteProfileAction->setEnabled(false);
        mProfilesComboBox->setEditEnabled(false);
    } else {
        mDeleteProfileAction->setEnabled(true);
        mProfilesComboBox->setEditEnabled(true);
    }

    if (!previous.isEmpty()) {
        writeConfig(previous);
        mLauncherConfig->sync();

        if (mProfilesComboBox->currentIndex() == -1)
            return;

    } else {
        return;
    }

    mDataFilesList->uncheckAll();
    readConfig();
}

void DataFilesPage::profileRenamed(const QString &previous, const QString &current)
{
    if (previous.isEmpty())
        return;

    // Save the new profile name
    writeConfig(current);

    // Make sure we have no groups open
     while (!mLauncherConfig->group().isEmpty()) {
         mLauncherConfig->endGroup();
     }

     mLauncherConfig->beginGroup("Profiles");

     // Open the profile-name subgroup
     mLauncherConfig->beginGroup(previous);
     mLauncherConfig->remove(""); // Clear the subgroup
     mLauncherConfig->endGroup();
     mLauncherConfig->endGroup();
     mLauncherConfig->sync();

     // Remove the profile from the combobox
     mProfilesComboBox->removeItem(mProfilesComboBox->findText(previous));

     mDataFilesList->uncheckAll();
     ////mMastersModel->uncheckAll();
     ////mPluginsModel->uncheckAll();
     readConfig();
}
