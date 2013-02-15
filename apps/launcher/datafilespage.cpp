#include <QtGui>

#include <components/esm/esmreader.hpp>
#include <components/files/configurationmanager.hpp>

#include "model/datafilesmodel.hpp"
#include "model/esm/esmfile.hpp"

#include "settings/gamesettings.hpp"
#include "settings/launchersettings.hpp"

#include "utils/profilescombobox.hpp"
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

DataFilesPage::DataFilesPage(Files::ConfigurationManager &cfg, GameSettings &gameSettings, LauncherSettings &launcherSettings, QWidget *parent)
    : mCfgMgr(cfg)
    , mGameSettings(gameSettings)
    , mLauncherSettings(launcherSettings)
    , QWidget(parent)
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
    mSplitter = new QSplitter(this);
    mSplitter->setOrientation(Qt::Horizontal);
    mSplitter->setChildrenCollapsible(false); // Don't allow the widgets to be hidden
    mSplitter->addWidget(mMastersTable);
    mSplitter->addWidget(mPluginsTable);

    // Adjust the default widget widths inside the splitter
    QList<int> sizeList;
    sizeList << mLauncherSettings.value(QString("General/MastersTable/width"), QString("200")).toInt();
    sizeList << mLauncherSettings.value(QString("General/PluginTable/width"), QString("340")).toInt();
    qDebug() << sizeList;

    mSplitter->setSizes(sizeList);

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
    pageLayout->addWidget(mSplitter);
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

    connect(mSplitter, SIGNAL(splitterMoved(int,int)), this, SLOT(updateSplitter()));

    createActions();
    setupDataFiles();
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

void DataFilesPage::readConfig()
{
//    // Don't read the config if no masters are found
//    if (mMastersModel->rowCount() < 1)
//        return;

//    QString profile = mProfilesComboBox->currentText();

//    // Make sure we have no groups open
//    while (!mLauncherConfig->group().isEmpty()) {
//        mLauncherConfig->endGroup();
//    }

//    mLauncherConfig->beginGroup("Profiles");
//    mLauncherConfig->beginGroup(profile);

//    QStringList childKeys = mLauncherConfig->childKeys();
//    QStringList plugins;

//    // Sort the child keys numerical instead of alphabetically
//    // i.e. Plugin1, Plugin2 instead of Plugin1, Plugin10
//    qSort(childKeys.begin(), childKeys.end(), naturalSortLessThanCI);

//    foreach (const QString &key, childKeys) {
//        const QString keyValue = mLauncherConfig->value(key).toString();

//        if (key.startsWith("Plugin")) {
//            //QStringList checked = mPluginsModel->checkedItems();
//            EsmFile *file = mPluginsModel->findItem(keyValue);
//            QModelIndex index = mPluginsModel->indexFromItem(file);

//            mPluginsModel->setCheckState(index, Qt::Checked);
//            // Move the row to the top of te view
//            //mPluginsModel->moveRow(index.row(), checked.count());
//            plugins << keyValue;
//        }

//        if (key.startsWith("Master")) {
//            EsmFile *file = mMastersModel->findItem(keyValue);
//            mMastersModel->setCheckState(mMastersModel->indexFromItem(file), Qt::Checked);
//        }
//    }

//    qDebug() << plugins;
}

void DataFilesPage::setupDataFiles()
{
    // Set the encoding to the one found in openmw.cfg or the default
    mMastersModel->setEncoding(mGameSettings.value(QString("encoding"), QString("win1252")));
    mPluginsModel->setEncoding(mGameSettings.value(QString("encoding"), QString("win1252")));

    QStringList paths = mGameSettings.getDataDirs();

    foreach (const QString &path, paths) {
        mMastersModel->addMasters(path);
        mPluginsModel->addPlugins(path);
    }

    QString dataLocal = mGameSettings.getDataLocal();
    if (!dataLocal.isEmpty()) {
        mMastersModel->addMasters(dataLocal);
        mPluginsModel->addPlugins(dataLocal);
    }

    QStringList profiles = mLauncherSettings.subKeys(QString("Profiles/"));
    QString profile = mLauncherSettings.value(QString("Profiles/CurrentProfile"));

    mProfilesComboBox->addItems(profiles);

    // Add the current profile if empty
    if (mProfilesComboBox->findText(profile) == -1)
        mProfilesComboBox->addItem(profile);

    if (mProfilesComboBox->findText(QString("Default")) == -1)
        mProfilesComboBox->addItem(QString("Default"));

    if (profile.isEmpty()) {
        mProfilesComboBox->setCurrentIndex(mProfilesComboBox->findText(QString("Default")));
    } else {
        mProfilesComboBox->setCurrentIndex(mProfilesComboBox->findText(profile));
    }

    loadSettings();
}

void DataFilesPage::loadSettings()
{
    qDebug() << "load settings";
    QString profile = mLauncherSettings.value(QString("Profiles/CurrentProfile"));

    qDebug() << mLauncherSettings.values(QString("Profiles/Default"), Qt::MatchStartsWith);



    if (profile.isEmpty())
        return;


    mMastersModel->uncheckAll();
    mPluginsModel->uncheckAll();

    QStringList masters = mLauncherSettings.values(QString("Profiles/") + profile + QString("/master"), Qt::MatchExactly);
    QStringList plugins = mLauncherSettings.values(QString("Profiles/") + profile + QString("/plugin"), Qt::MatchExactly);
    qDebug() << "masters to check " << plugins;

    foreach (const QString &master, masters) {
        QModelIndex index = mMastersModel->indexFromItem(mMastersModel->findItem(master));
        if (index.isValid())
            mMastersModel->setCheckState(index, Qt::Checked);
    }

    foreach (const QString &plugin, plugins) {
        QModelIndex index = mPluginsModel->indexFromItem(mPluginsModel->findItem(plugin));
        if (index.isValid())
            mPluginsModel->setCheckState(index, Qt::Checked);
    }
}

void DataFilesPage::saveSettings()
{
    QString profile = mLauncherSettings.value(QString("Profiles/CurrentProfile"));

    if (profile.isEmpty()) {
        profile = mProfilesComboBox->currentText();
        mLauncherSettings.setValue(QString("Profiles/CurrentProfile"), profile);
    }

    qDebug() << "save settings" << profile;
    mLauncherSettings.remove(QString("Profiles/") + profile + QString("/master"));
    mLauncherSettings.remove(QString("Profiles/") + profile + QString("/plugin"));

    QStringList items = mMastersModel->checkedItems();

    foreach(const QString &master, items) {
        qDebug() << "setting " << master;
        mLauncherSettings.setMultiValue(QString("Profiles/") + profile + QString("/master"), master);
    }

    items.clear();
    items = mPluginsModel->checkedItems();

    qDebug() << items.size();

    foreach(const QString &plugin, items) {
        qDebug() << "setting " << plugin;
        mLauncherSettings.setMultiValue(QString("Profiles/") + profile + QString("/plugin"), plugin);
    }



}

void DataFilesPage::writeConfig(QString profile)
{


//    // Don't overwrite the config if no masters are found
//    if (mMastersModel->rowCount() < 1)
//        return;

//    QString pathStr = QString::fromStdString(mCfgMgr.getUserPath().string());
//    QDir userPath(pathStr);

//    if (!userPath.exists()) {
//        if (!userPath.mkpath(pathStr)) {
//            QMessageBox msgBox;
//            msgBox.setWindowTitle("Error creating OpenMW configuration directory");
//            msgBox.setIcon(QMessageBox::Critical);
//            msgBox.setStandardButtons(QMessageBox::Ok);
//            msgBox.setText(tr("<br><b>Could not create %0</b><br><br> \
//                              Please make sure you have the right permissions and try again.<br>").arg(pathStr));
//            msgBox.exec();

//            qApp->quit();
//            return;
//        }
//    }
//    // Open the OpenMW config as a QFile
//    QFile file(pathStr.append("openmw.cfg"));

//    if (!file.open(QIODevice::ReadWrite | QIODevice::Text)) {
//        // File cannot be opened or created
//        QMessageBox msgBox;
//        msgBox.setWindowTitle("Error writing OpenMW configuration file");
//        msgBox.setIcon(QMessageBox::Critical);
//        msgBox.setStandardButtons(QMessageBox::Ok);
//        msgBox.setText(tr("<br><b>Could not open or create %0</b><br><br> \
//                          Please make sure you have the right permissions and try again.<br>").arg(file.fileName()));
//        msgBox.exec();

//        qApp->quit();
//        return;
//    }

//    QTextStream in(&file);
//    QByteArray buffer;

//    // Remove all previous entries from config
//    while (!in.atEnd()) {
//        QString line = in.readLine();
//        if (!line.startsWith("master") &&
//            !line.startsWith("plugin") &&
//            !line.startsWith("data") &&
//            !line.startsWith("data-local"))
//        {
//            buffer += line += "\n";
//        }
//    }

//    file.close();

//    // Now we write back the other config entries
//    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
//        QMessageBox msgBox;
//        msgBox.setWindowTitle("Error writing OpenMW configuration file");
//        msgBox.setIcon(QMessageBox::Critical);
//        msgBox.setStandardButtons(QMessageBox::Ok);
//        msgBox.setText(tr("<br><b>Could not write to %0</b><br><br> \
//                          Please make sure you have the right permissions and try again.<br>").arg(file.fileName()));
//        msgBox.exec();

//        qApp->quit();
//        return;
//    }

//    if (!buffer.isEmpty()) {
//        file.write(buffer);
//    }

//    QTextStream gameConfig(&file);


//    QString path;

//    // data= directories
//    for (Files::PathContainer::iterator it = mDataDirs.begin(); it != mDataDirs.end(); ++it) {
//        path = QString::fromStdString(it->string());
//        path.remove(QChar('\"'));

//        // Make sure the string is quoted when it contains spaces
//        if (path.contains(" ")) {
//            gameConfig << "data=\"" << path << "\"" << endl;
//        } else {
//            gameConfig << "data=" << path << endl;
//        }
//    }

//    // data-local directory
//    if (!mDataLocal.empty()) {
//        path = QString::fromStdString(mDataLocal.front().string());
//        path.remove(QChar('\"'));

//        if (path.contains(" ")) {
//            gameConfig << "data-local=\"" << path << "\"" << endl;
//        } else {
//            gameConfig << "data-local=" << path << endl;
//        }
//    }


//    if (profile.isEmpty())
//        profile = mProfilesComboBox->currentText();

//    if (profile.isEmpty())
//        return;

//    // Make sure we have no groups open
//    while (!mLauncherConfig->group().isEmpty()) {
//        mLauncherConfig->endGroup();
//    }

//    mLauncherConfig->beginGroup("Profiles");
//    mLauncherConfig->setValue("CurrentProfile", profile);

//    // Open the profile-name subgroup
//    mLauncherConfig->beginGroup(profile);
//    mLauncherConfig->remove(""); // Clear the subgroup

//    // Now write the masters to the configs
//    const QStringList masters = mMastersModel->checkedItems();

//    // We don't use foreach because we need i
//    for (int i = 0; i < masters.size(); ++i) {
//        const QString currentMaster = masters.at(i);

//        mLauncherConfig->setValue(QString("Master%0").arg(i), currentMaster);
//        gameConfig << "master=" << currentMaster << endl;

//    }

//    // And finally write all checked plugins
//    const QStringList plugins = mPluginsModel->checkedItems();

//    for (int i = 0; i < plugins.size(); ++i) {
//        const QString currentPlugin = plugins.at(i);
//        mLauncherConfig->setValue(QString("Plugin%1").arg(i), currentPlugin);
//        gameConfig << "plugin=" << currentPlugin << endl;
//    }

//    file.close();
//    mLauncherConfig->endGroup();
//    mLauncherConfig->endGroup();
//    mLauncherConfig->sync();
}


void DataFilesPage::newProfile()
{
    if (mNewProfileDialog->exec() == QDialog::Accepted) {
        QString profile = mNewProfileDialog->lineEdit()->text();
        mProfilesComboBox->addItem(profile);
        mProfilesComboBox->setCurrentIndex(mProfilesComboBox->findText(profile));
    }
}

void DataFilesPage::updateOkButton(const QString &text)
{
    // We do this here because we need the profiles combobox text
    if (text.isEmpty()) {
         mNewProfileDialog->setOkButtonEnabled(false);
         return;
    }

    (mProfilesComboBox->findText(text) == -1)
            ? mNewProfileDialog->setOkButtonEnabled(true)
            : mNewProfileDialog->setOkButtonEnabled(false);
}

void DataFilesPage::updateSplitter()
{
    // Sigh, update the saved splitter size in settings only when moved
    // Since getting mSplitter->sizes() if page is hidden returns invalid values
    QList<int> sizes = mSplitter->sizes();

    mLauncherSettings.setValue(QString("General/MastersTable/width"), QString::number(sizes.at(0)));
    mLauncherSettings.setValue(QString("General/PluginsTable/width"), QString::number(sizes.at(1)));
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
        mLauncherSettings.remove(QString("Profiles/") + profile + QString("/master"));
        mLauncherSettings.remove(QString("Profiles/") + profile + QString("/plugin"));

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

    if (previous.isEmpty())
        return;

    if (mProfilesComboBox->findText(previous) == -1)
        return; // Profile was deleted

    // Store the previous profile
    mLauncherSettings.setValue(QString("Profiles/CurrentProfile"), previous);
    saveSettings();
    mLauncherSettings.setValue(QString("Profiles/CurrentProfile"), current);


    mMastersModel->uncheckAll();
    mPluginsModel->uncheckAll();
    loadSettings();
}

void DataFilesPage::profileRenamed(const QString &previous, const QString &current)
{
    qDebug() << "rename";
    if (previous.isEmpty())
        return;

    // Save the new profile name
    mLauncherSettings.setValue(QString("Profiles/CurrentProfile"), current);
    saveSettings();

    // Remove the old one
    mLauncherSettings.remove(QString("Profiles/") + previous + QString("/master"));
    mLauncherSettings.remove(QString("Profiles/") + previous + QString("/plugin"));

    // Remove the profile from the combobox
    mProfilesComboBox->removeItem(mProfilesComboBox->findText(previous));

    mMastersModel->uncheckAll();
    mPluginsModel->uncheckAll();
    loadSettings();
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
