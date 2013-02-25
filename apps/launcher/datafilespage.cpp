#include "datafilespage.hpp"

#include <QtGui>

#include <components/esm/esmreader.hpp>
#include <components/files/configurationmanager.hpp>

#include <components/fileorderlist/model/datafilesmodel.hpp>
#include <components/fileorderlist/model/esm/esmfile.hpp>

#include <components/fileorderlist/utils/lineedit.hpp>
#include <components/fileorderlist/utils/naturalsort.hpp>

#include "model/pluginsproxymodel.hpp"

#include "settings/gamesettings.hpp"
#include "settings/launchersettings.hpp"

#include "utils/profilescombobox.hpp"
#include "utils/textinputdialog.hpp"

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
    mDataFilesModel = new DataFilesModel(this);

    mMastersProxyModel = new QSortFilterProxyModel();
    mMastersProxyModel->setFilterRegExp(QString("^.*\\.esm"));
    mMastersProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    mMastersProxyModel->setSourceModel(mDataFilesModel);

    mPluginsProxyModel = new PluginsProxyModel();
    mPluginsProxyModel->setFilterRegExp(QString("^.*\\.esp"));
    mPluginsProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    mPluginsProxyModel->setSourceModel(mDataFilesModel);

    mFilterProxyModel = new QSortFilterProxyModel();
    mFilterProxyModel->setDynamicSortFilter(true);
    mFilterProxyModel->setSourceModel(mPluginsProxyModel);

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
    mMastersTable->setModel(mMastersProxyModel);
    mMastersTable->setObjectName("MastersTable");
    mMastersTable->setContextMenuPolicy(Qt::CustomContextMenu);
    mMastersTable->setSortingEnabled(false);
    mMastersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mMastersTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mMastersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mMastersTable->setAlternatingRowColors(true);
    mMastersTable->horizontalHeader()->setStretchLastSection(true);
    mMastersTable->horizontalHeader()->hide();

    // Set the row height to the size of the checkboxes
    mMastersTable->verticalHeader()->setDefaultSectionSize(height);
    mMastersTable->verticalHeader()->setResizeMode(QHeaderView::Fixed);
    mMastersTable->verticalHeader()->hide();

    mPluginsTable = new QTableView(this);
    mPluginsTable->setModel(mFilterProxyModel);
    mPluginsTable->setObjectName("PluginsTable");
    mPluginsTable->setContextMenuPolicy(Qt::CustomContextMenu);
    mPluginsTable->setSortingEnabled(false);
    mPluginsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mPluginsTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mPluginsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mPluginsTable->setAlternatingRowColors(true);
    mPluginsTable->setVerticalScrollMode(QAbstractItemView::ScrollPerItem);
    mPluginsTable->horizontalHeader()->setStretchLastSection(true);
    mPluginsTable->horizontalHeader()->hide();

    mPluginsTable->verticalHeader()->setDefaultSectionSize(height);
    mPluginsTable->verticalHeader()->setResizeMode(QHeaderView::Fixed);

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

    connect(mPluginsTable, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
    connect(mMastersTable, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));

    connect(mDataFilesModel, SIGNAL(layoutChanged()), this, SLOT(updateViews()));
    
    connect(filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterChanged(QString)));

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
    mCheckAction = new QAction(tr("Check Selection"), this);
    connect(mCheckAction, SIGNAL(triggered()), this, SLOT(check()));

    mUncheckAction = new QAction(tr("Uncheck Selection"), this);
    connect(mUncheckAction, SIGNAL(triggered()), this, SLOT(uncheck()));

    mContextMenu = new QMenu(this);
    mContextMenu->addAction(mCheckAction);
    mContextMenu->addAction(mUncheckAction);
}

void DataFilesPage::setupDataFiles()
{
    // Set the encoding to the one found in openmw.cfg or the default
    mDataFilesModel->setEncoding(mGameSettings.value(QString("encoding"), QString("win1252")));

    QStringList paths = mGameSettings.getDataDirs();

    foreach (const QString &path, paths) {
        mDataFilesModel->addFiles(path);
    }

    QString dataLocal = mGameSettings.getDataLocal();
    if (!dataLocal.isEmpty())
        mDataFilesModel->addFiles(dataLocal);

    // Sort by date accessed for now
    mDataFilesModel->sort(3);

    QStringList profiles = mLauncherSettings.subKeys(QString("Profiles/"));
    QString profile = mLauncherSettings.value(QString("Profiles/currentprofile"));

    if (!profiles.isEmpty())
        mProfilesComboBox->addItems(profiles);

    // Add the current profile if empty
    if (mProfilesComboBox->findText(profile) == -1)
        mProfilesComboBox->addItem(profile);

    if (mProfilesComboBox->findText(QString("Default")) == -1)
        mProfilesComboBox->addItem(QString("Default"));

    if (profile.isEmpty() || profile == QLatin1String("Default")) {
        mProfilesComboBox->setCurrentIndex(mProfilesComboBox->findText(QString("Default")));
    } else {
        mProfilesComboBox->setEditEnabled(true);
        mProfilesComboBox->setCurrentIndex(mProfilesComboBox->findText(profile));
    }

    // We do this here to prevent deletion of profiles when initializing the combobox
    connect(mProfilesComboBox, SIGNAL(profileRenamed(QString,QString)), this, SLOT(profileRenamed(QString,QString)));
    connect(mProfilesComboBox, SIGNAL(profileChanged(QString,QString)), this, SLOT(profileChanged(QString,QString)));

    loadSettings();

}

void DataFilesPage::loadSettings()
{
    QString profile = mLauncherSettings.value(QString("Profiles/currentprofile"));

    if (profile.isEmpty())
        return;

    mDataFilesModel->uncheckAll();

    QStringList masters = mLauncherSettings.values(QString("Profiles/") + profile + QString("/master"), Qt::MatchExactly);
    QStringList plugins = mLauncherSettings.values(QString("Profiles/") + profile + QString("/plugin"), Qt::MatchExactly);

    foreach (const QString &master, masters) {
        QModelIndex index = mDataFilesModel->indexFromItem(mDataFilesModel->findItem(master));
        if (index.isValid())
            mDataFilesModel->setCheckState(index, Qt::Checked);
    }

    foreach (const QString &plugin, plugins) {
        QModelIndex index = mDataFilesModel->indexFromItem(mDataFilesModel->findItem(plugin));
        if (index.isValid())
            mDataFilesModel->setCheckState(index, Qt::Checked);
    }
}

void DataFilesPage::saveSettings()
{
    if (mDataFilesModel->rowCount() < 1)
        return;

    QString profile = mLauncherSettings.value(QString("Profiles/currentprofile"));

    if (profile.isEmpty()) {
        profile = mProfilesComboBox->currentText();
        mLauncherSettings.setValue(QString("Profiles/currentprofile"), profile);
    }

    mLauncherSettings.remove(QString("Profiles/") + profile + QString("/master"));
    mLauncherSettings.remove(QString("Profiles/") + profile + QString("/plugin"));

    mGameSettings.remove(QString("master"));
    mGameSettings.remove(QString("plugin"));

    QStringList items = mDataFilesModel->checkedItems();

    foreach(const QString &item, items) {

        if (item.endsWith(QString(".esm"), Qt::CaseInsensitive)) {
            mLauncherSettings.setMultiValue(QString("Profiles/") + profile + QString("/master"), item);
            mGameSettings.setMultiValue(QString("master"), item);

        } else if (item.endsWith(QString(".esp"), Qt::CaseInsensitive)) {
            mLauncherSettings.setMultiValue(QString("Profiles/") + profile + QString("/plugin"), item);
            mGameSettings.setMultiValue(QString("plugin"), item);
        }
    }

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

void DataFilesPage::updateViews()
{
    // Ensure the columns are hidden because sort() re-enables them
    mMastersTable->setColumnHidden(1, true);
    mMastersTable->setColumnHidden(2, true);
    mMastersTable->setColumnHidden(3, true);
    mMastersTable->setColumnHidden(4, true);
    mMastersTable->setColumnHidden(5, true);
    mMastersTable->setColumnHidden(6, true);
    mMastersTable->setColumnHidden(7, true);
    mMastersTable->setColumnHidden(8, true);

    mPluginsTable->setColumnHidden(1, true);
    mPluginsTable->setColumnHidden(2, true);
    mPluginsTable->setColumnHidden(3, true);
    mPluginsTable->setColumnHidden(4, true);
    mPluginsTable->setColumnHidden(5, true);
    mPluginsTable->setColumnHidden(6, true);
    mPluginsTable->setColumnHidden(7, true);
    mPluginsTable->setColumnHidden(8, true);
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
    if (mPluginsTable->hasFocus())
        setPluginsCheckstates(Qt::Checked);

    if (mMastersTable->hasFocus())
        setMastersCheckstates(Qt::Checked);

}

void DataFilesPage::uncheck()
{
    if (mPluginsTable->hasFocus())
        setPluginsCheckstates(Qt::Unchecked);

    if (mMastersTable->hasFocus())
        setMastersCheckstates(Qt::Unchecked);
}

void DataFilesPage::refresh()
{
//    mDataFilesModel->sort(0);

    // Refresh the plugins table
    mPluginsTable->scrollToTop();
}

void DataFilesPage::setMastersCheckstates(Qt::CheckState state)
{
    if (!mMastersTable->selectionModel()->hasSelection()) {
        return;
    }

    QModelIndexList indexes = mMastersTable->selectionModel()->selectedIndexes();

    foreach (const QModelIndex &index, indexes)
    {
        if (!index.isValid())
            return;

        QModelIndex sourceIndex = mMastersProxyModel->mapToSource(index);

        if (!sourceIndex.isValid())
            return;

        mDataFilesModel->setCheckState(sourceIndex, state);
    }
}

void DataFilesPage::setPluginsCheckstates(Qt::CheckState state)
{
    if (!mPluginsTable->selectionModel()->hasSelection()) {
        return;
    }

    QModelIndexList indexes = mPluginsTable->selectionModel()->selectedIndexes();

    foreach (const QModelIndex &index, indexes)
    {
        if (!index.isValid())
            return;

        QModelIndex sourceIndex = mPluginsProxyModel->mapToSource(
                    mFilterProxyModel->mapToSource(index));

        if (!sourceIndex.isValid())
            return;

        mDataFilesModel->setCheckState(sourceIndex, state);
    }
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
        QModelIndex sourceIndex = mPluginsProxyModel->mapToSource(
                    mFilterProxyModel->mapToSource(index));

        if (sourceIndex.isValid()) {
            (mDataFilesModel->checkState(sourceIndex) == Qt::Checked)
                    ? mDataFilesModel->setCheckState(sourceIndex, Qt::Unchecked)
                    : mDataFilesModel->setCheckState(sourceIndex, Qt::Checked);
        }
    }

    if (object->objectName() == QLatin1String("MastersTable")) {
        QModelIndex sourceIndex = mMastersProxyModel->mapToSource(index);

        if (sourceIndex.isValid()) {
            (mDataFilesModel->checkState(sourceIndex) == Qt::Checked)
                    ? mDataFilesModel->setCheckState(sourceIndex, Qt::Unchecked)
                    : mDataFilesModel->setCheckState(sourceIndex, Qt::Checked);
        }
    }

    return;
}

void DataFilesPage::filterChanged(const QString filter)
{
    QRegExp regExp(filter, Qt::CaseInsensitive, QRegExp::FixedString);
    mFilterProxyModel->setFilterRegExp(regExp);
}

void DataFilesPage::profileChanged(const QString &previous, const QString &current)
{
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
    mLauncherSettings.setValue(QString("Profiles/currentprofile"), previous);
    saveSettings();
    mLauncherSettings.setValue(QString("Profiles/currentprofile"), current);

    loadSettings();
}

void DataFilesPage::profileRenamed(const QString &previous, const QString &current)
{
    if (previous.isEmpty())
        return;

    // Save the new profile name
    mLauncherSettings.setValue(QString("Profiles/currentprofile"), current);
    saveSettings();

    // Remove the old one
    mLauncherSettings.remove(QString("Profiles/") + previous + QString("/master"));
    mLauncherSettings.remove(QString("Profiles/") + previous + QString("/plugin"));

    // Remove the profile from the combobox
    mProfilesComboBox->removeItem(mProfilesComboBox->findText(previous));

    loadSettings();

}

void DataFilesPage::showContextMenu(const QPoint &point)
{
    QObject *object = QObject::sender();

    // Not a signal-slot call
    if (!object)
        return;

    if (object->objectName() == QLatin1String("PluginsTable")) {
        if (!mPluginsTable->selectionModel()->hasSelection())
            return;

        QPoint globalPos = mPluginsTable->mapToGlobal(point);
        QModelIndexList indexes = mPluginsTable->selectionModel()->selectedIndexes();

        // Show the check/uncheck actions depending on the state of the selected items
        mUncheckAction->setEnabled(false);
        mCheckAction->setEnabled(false);

        foreach (const QModelIndex &index, indexes)
        {
            if (!index.isValid())
                return;

            QModelIndex sourceIndex = mPluginsProxyModel->mapToSource(
                        mFilterProxyModel->mapToSource(index));

            if (!sourceIndex.isValid())
                return;

            (mDataFilesModel->checkState(sourceIndex) == Qt::Checked)
                    ? mUncheckAction->setEnabled(true)
                    : mCheckAction->setEnabled(true);
        }

        // Show menu
        mContextMenu->exec(globalPos);
    }

    if (object->objectName() == QLatin1String("MastersTable")) {
        if (!mMastersTable->selectionModel()->hasSelection())
            return;

        QPoint globalPos = mMastersTable->mapToGlobal(point);
        QModelIndexList indexes = mMastersTable->selectionModel()->selectedIndexes();

        // Show the check/uncheck actions depending on the state of the selected items
        mUncheckAction->setEnabled(false);
        mCheckAction->setEnabled(false);

        foreach (const QModelIndex &index, indexes)
        {
            if (!index.isValid())
                return;

            QModelIndex sourceIndex = mMastersProxyModel->mapToSource(index);

            if (!sourceIndex.isValid())
                return;

            (mDataFilesModel->checkState(sourceIndex) == Qt::Checked)
                    ? mUncheckAction->setEnabled(true)
                    : mCheckAction->setEnabled(true);
        }

        mContextMenu->exec(globalPos);
    }
}
