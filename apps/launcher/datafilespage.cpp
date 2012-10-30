#include <QtGui>

#include <components/esm/esmreader.hpp>
#include <components/files/configurationmanager.hpp>

#include "model/datafilesmodel.hpp"
#include "model/esm/esmfile.hpp"

#include "utils/profilescombobox.hpp"
#include "utils/filedialog.hpp"
#include "utils/lineedit.hpp"
#include "utils/naturalsort.hpp"
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

//sort QModelIndexList ascending
bool rowGreaterThan(const QModelIndex &index1, const QModelIndex &index2)
{
    return index1.row() >= index2.row();
}

//sort QModelIndexList descending
bool rowSmallerThan(const QModelIndex &index1, const QModelIndex &index2)
{
    return index1.row() <= index2.row();
}

DataFilesPage::DataFilesPage(Files::ConfigurationManager &cfg, QWidget *parent)
    : QWidget(parent)
    , mCfgMgr(cfg)
{
    // Models
    mMastersModel = new DataFilesModel(this);
    mPluginsModel = new DataFilesModel(this);

    mPluginsProxyModel = new QSortFilterProxyModel();
    mPluginsProxyModel->setDynamicSortFilter(true);
    mPluginsProxyModel->setSourceModel(mPluginsModel);

    // Filter toolbar
    QLabel *filterLabel = new QLabel(tr("&Filter:"), this);
    LineEdit *filterLineEdit = new LineEdit(this);
    filterLabel->setBuddy(filterLineEdit);

    QToolBar *filterToolBar = new QToolBar(this);
    filterToolBar->setMovable(false);

    // Create a container widget and a layout to get the spacer to work
    QWidget *filterWidget = new QWidget(this);
    QHBoxLayout *filterLayout = new QHBoxLayout(filterWidget);
    QSpacerItem *hSpacer1 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    filterLayout->addItem(hSpacer1);
    filterLayout->addWidget(filterLabel);
    filterLayout->addWidget(filterLineEdit);

    filterToolBar->addWidget(filterWidget);

    QCheckBox checkBox;
    unsigned int height = checkBox.sizeHint().height() + 4;

    mMastersTable = new QTableView(this);
    mMastersTable->setModel(mMastersModel);
    mMastersTable->setObjectName("MastersTable");
    mMastersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mMastersTable->setSelectionMode(QAbstractItemView::SingleSelection);
    mMastersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mMastersTable->setAlternatingRowColors(true);
    mMastersTable->horizontalHeader()->setStretchLastSection(true);
    mMastersTable->horizontalHeader()->hide();

    // Set the row height to the size of the checkboxes
    mMastersTable->verticalHeader()->setDefaultSectionSize(height);
    mMastersTable->verticalHeader()->setResizeMode(QHeaderView::Fixed);
    mMastersTable->verticalHeader()->hide();
    mMastersTable->setColumnHidden(1, true);
    mMastersTable->setColumnHidden(2, true);
    mMastersTable->setColumnHidden(3, true);
    mMastersTable->setColumnHidden(4, true);
    mMastersTable->setColumnHidden(5, true);
    mMastersTable->setColumnHidden(6, true);
    mMastersTable->setColumnHidden(7, true);
    mMastersTable->setColumnHidden(8, true);

    mPluginsTable = new QTableView(this);
    mPluginsTable->setModel(mPluginsProxyModel);
    mPluginsTable->setObjectName("PluginsTable");
    mPluginsTable->setContextMenuPolicy(Qt::CustomContextMenu);
    mPluginsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mPluginsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    mPluginsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mPluginsTable->setAlternatingRowColors(true);
    mPluginsTable->setVerticalScrollMode(QAbstractItemView::ScrollPerItem);
    mPluginsTable->horizontalHeader()->setStretchLastSection(true);
    mPluginsTable->horizontalHeader()->hide();

    mPluginsTable->verticalHeader()->setDefaultSectionSize(height);
    mPluginsTable->verticalHeader()->setResizeMode(QHeaderView::Fixed);
    mPluginsTable->setColumnHidden(1, true);
    mPluginsTable->setColumnHidden(2, true);
    mPluginsTable->setColumnHidden(3, true);
    mPluginsTable->setColumnHidden(4, true);
    mPluginsTable->setColumnHidden(5, true);
    mPluginsTable->setColumnHidden(6, true);
    mPluginsTable->setColumnHidden(7, true);
    mPluginsTable->setColumnHidden(8, true);

    // Add both tables to a splitter
    QSplitter *splitter = new QSplitter(this);
    splitter->setOrientation(Qt::Horizontal);
    splitter->addWidget(mMastersTable);
    splitter->addWidget(mPluginsTable);

    // Adjust the default widget widths inside the splitter
    QList<int> sizeList;
    sizeList << 175 << 200;
    splitter->setSizes(sizeList);

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

    pageLayout->addWidget(filterToolBar);
    pageLayout->addWidget(splitter);
    pageLayout->addWidget(mProfileToolBar);

    // Create a dialog for the new profile name input
    mNewProfileDialog = new TextInputDialog(tr("New Profile"), tr("Profile name:"), this);

    connect(mNewProfileDialog->lineEdit(), SIGNAL(textChanged(QString)), this, SLOT(updateOkButton(QString)));

    connect(mPluginsTable, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(setCheckState(QModelIndex)));
    connect(mMastersTable, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(setCheckState(QModelIndex)));

    connect(mMastersModel, SIGNAL(checkedItemsChanged(QStringList,QStringList)), mPluginsModel, SLOT(slotcheckedItemsChanged(QStringList,QStringList)));

    connect(filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterChanged(QString)));

    connect(mPluginsTable, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));

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

    // Context menu actions
    mCheckAction = new QAction(tr("Check selected"), this);
    connect(mCheckAction, SIGNAL(triggered()), this, SLOT(check()));

    mUncheckAction = new QAction(tr("Uncheck selected"), this);
    connect(mUncheckAction, SIGNAL(triggered()), this, SLOT(uncheck()));

    // Context menu for the plugins table
    mContextMenu = new QMenu(this);

    mContextMenu->addAction(mCheckAction);
    mContextMenu->addAction(mUncheckAction);

}

void DataFilesPage::setupConfig()
{
    QString config = QString::fromStdString((mCfgMgr.getLocalPath() / "launcher.cfg").string());
    QFile file(config);

    if (!file.exists()) {
        config = QString::fromStdString((mCfgMgr.getUserPath() / "launcher.cfg").string());
    }

    // Open our config file
    mLauncherConfig = new QSettings(config, QSettings::IniFormat);
    file.close();

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
    // Don't read the config if no masters are found
    if (mMastersModel->rowCount() < 1)
        return;

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

        if (key.startsWith("Plugin")) {
            //QStringList checked = mPluginsModel->checkedItems();
            EsmFile *file = mPluginsModel->findItem(keyValue);
            QModelIndex index = mPluginsModel->indexFromItem(file);

            mPluginsModel->setCheckState(index, Qt::Checked);
            // Move the row to the top of te view
            //mPluginsModel->moveRow(index.row(), checked.count());
            plugins << keyValue;
        }

        if (key.startsWith("Master")) {
            EsmFile *file = mMastersModel->findItem(keyValue);
            mMastersModel->setCheckState(mMastersModel->indexFromItem(file), Qt::Checked);
        }
    }

    qDebug() << plugins;


//    // Set the checked item positions
//    const QStringList checked = mPluginsModel->checkedItems();
//    for (int i = 0; i < plugins.size(); ++i) {
//        EsmFile *file = mPluginsModel->findItem(plugins.at(i));
//        QModelIndex index = mPluginsModel->indexFromItem(file);
//        mPluginsModel->moveRow(index.row(), i);
//        qDebug() << "Moving: " << plugins.at(i) << " from: " << index.row() << " to: " << i << " count: " << checked.count();

//    }

    // Iterate over the plugins to set their checkstate and position
//    for (int i = 0; i < plugins.size(); ++i) {
//        const QString plugin = plugins.at(i);

//        const QList<QStandardItem *> pluginList = mPluginsModel->findItems(plugin);

//        if (!pluginList.isEmpty())
//        {
//            foreach (const QStandardItem *currentPlugin, pluginList) {
//                mPluginsModel->setData(currentPlugin->index(), Qt::Checked, Qt::CheckStateRole);

//                // Move the plugin to the position specified in the config file
//                mPluginsModel->insertRow(i, mPluginsModel->takeRow(currentPlugin->row()));
//            }
//        }
//    }

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

    // Add the paths to the respective models
    for (Files::PathContainer::iterator it = mDataDirs.begin(); it != mDataDirs.end(); ++it) {
        QString path = QString::fromStdString(it->string());
        path.remove(QChar('\"'));
        mMastersModel->addMasters(path);
        mPluginsModel->addPlugins(path);
    }

    // Same for the data-local paths
    for (Files::PathContainer::iterator it = mDataLocal.begin(); it != mDataLocal.end(); ++it) {
        QString path = QString::fromStdString(it->string());
        path.remove(QChar('\"'));
        mMastersModel->addMasters(path);
        mPluginsModel->addPlugins(path);
    }

    mMastersModel->sort(0);
    mPluginsModel->sort(0);
//    mMastersTable->sortByColumn(3, Qt::AscendingOrder);
//    mPluginsTable->sortByColumn(3, Qt::AscendingOrder);


    readConfig();
    return true;
}

void DataFilesPage::writeConfig(QString profile)
{
    // Don't overwrite the config if no masters are found
    if (mMastersModel->rowCount() < 1)
        return;

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
    const QStringList masters = mMastersModel->checkedItems();

    // We don't use foreach because we need i
    for (int i = 0; i < masters.size(); ++i) {
        const QString currentMaster = masters.at(i);

        mLauncherConfig->setValue(QString("Master%0").arg(i), currentMaster);
        gameConfig << "master=" << currentMaster << endl;

    }

    // And finally write all checked plugins
    const QStringList plugins = mPluginsModel->checkedItems();

    for (int i = 0; i < plugins.size(); ++i) {
        const QString currentPlugin = plugins.at(i);
        mLauncherConfig->setValue(QString("Plugin%1").arg(i), currentPlugin);
        gameConfig << "plugin=" << currentPlugin << endl;
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

void DataFilesPage::check()
{
    // Check the current selection
    if (!mPluginsTable->selectionModel()->hasSelection()) {
        return;
    }

    QModelIndexList indexes = mPluginsTable->selectionModel()->selectedIndexes();

    //sort selection ascending because selectedIndexes returns an unsorted list
    //qSort(indexes.begin(), indexes.end(), rowSmallerThan);

    foreach (const QModelIndex &index, indexes) {
        if (!index.isValid())
            return;

        mPluginsModel->setCheckState(index, Qt::Checked);
    }
}

void DataFilesPage::uncheck()
{
    // uncheck the current selection
    if (!mPluginsTable->selectionModel()->hasSelection()) {
        return;
    }

    QModelIndexList indexes = mPluginsTable->selectionModel()->selectedIndexes();

    //sort selection ascending because selectedIndexes returns an unsorted list
    //qSort(indexes.begin(), indexes.end(), rowSmallerThan);

    foreach (const QModelIndex &index, indexes) {
        if (!index.isValid())
            return;

        mPluginsModel->setCheckState(index, Qt::Unchecked);
    }
}

void DataFilesPage::refresh()
{
    mPluginsModel->sort(0);


    // Refresh the plugins table
    mPluginsTable->scrollToTop();
    writeConfig();
    readConfig();
}


void DataFilesPage::setCheckState(QModelIndex index)
{
    if (!index.isValid())
        return;

    QObject *object = QObject::sender();

    // Not a signal-slot call
    if (!object)
        return;

    if (object->objectName() == QLatin1String("PluginsTable")) {
        QModelIndex sourceIndex = mPluginsProxyModel->mapToSource(index);

        (mPluginsModel->checkState(sourceIndex) == Qt::Checked)
                ? mPluginsModel->setCheckState(sourceIndex, Qt::Unchecked)
                : mPluginsModel->setCheckState(sourceIndex, Qt::Checked);
    }

    if (object->objectName() == QLatin1String("MastersTable")) {
        (mMastersModel->checkState(index) == Qt::Checked)
                ? mMastersModel->setCheckState(index, Qt::Unchecked)
                : mMastersModel->setCheckState(index, Qt::Checked);
    }

    return;

}

void DataFilesPage::filterChanged(const QString filter)
{
    QRegExp regExp(filter, Qt::CaseInsensitive, QRegExp::FixedString);
    mPluginsProxyModel->setFilterRegExp(regExp);
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

    mMastersModel->uncheckAll();
    mPluginsModel->uncheckAll();
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

     mMastersModel->uncheckAll();
     mPluginsModel->uncheckAll();
     readConfig();
}

void DataFilesPage::showContextMenu(const QPoint &point)
{
    // Make sure there are plugins in the view
    if (!mPluginsTable->selectionModel()->hasSelection()) {
        return;
    }

    QPoint globalPos = mPluginsTable->mapToGlobal(point);

    QModelIndexList indexes = mPluginsTable->selectionModel()->selectedIndexes();

    // Show the check/uncheck actions depending on the state of the selected items
    mUncheckAction->setEnabled(false);
    mCheckAction->setEnabled(false);

    foreach (const QModelIndex &index, indexes) {
        if (!index.isValid())
            return;

         (mPluginsModel->checkState(index) == Qt::Checked)
             ? mUncheckAction->setEnabled(true)
             : mCheckAction->setEnabled(true);
    }

    // Show menu
    mContextMenu->exec(globalPos);
}
