#include "datafilespage.hpp"

#include <QPushButton>
#include <QMessageBox>
#include <QCheckBox>
#include <QMenu>

#include <components/files/configurationmanager.hpp>

#include <components/fileorderlist/model/datafilesmodel.hpp>
#include <components/fileorderlist/model/pluginsproxymodel.hpp>
#include <components/fileorderlist/model/esm/esmfile.hpp>

#include <components/fileorderlist/utils/lineedit.hpp>
#include <components/fileorderlist/utils/naturalsort.hpp>
#include <components/fileorderlist/utils/profilescombobox.hpp>

#include "settings/gamesettings.hpp"
#include "settings/launchersettings.hpp"

#include "utils/textinputdialog.hpp"

DataFilesPage::DataFilesPage(Files::ConfigurationManager &cfg, GameSettings &gameSettings, LauncherSettings &launcherSettings, QWidget *parent)
    : mCfgMgr(cfg)
    , mGameSettings(gameSettings)
    , mLauncherSettings(launcherSettings)
    , QWidget(parent)
{
    setupUi(this);

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

    QCheckBox checkBox;
    unsigned int height = checkBox.sizeHint().height() + 4;

    mastersTable->setModel(mMastersProxyModel);
    mastersTable->setObjectName("MastersTable");
    mastersTable->setContextMenuPolicy(Qt::CustomContextMenu);
    mastersTable->setSortingEnabled(false);
    mastersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mastersTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mastersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mastersTable->setAlternatingRowColors(true);
    mastersTable->horizontalHeader()->setStretchLastSection(true);
    mastersTable->horizontalHeader()->hide();

    // Set the row height to the size of the checkboxes
    mastersTable->verticalHeader()->setDefaultSectionSize(height);
    mastersTable->verticalHeader()->setResizeMode(QHeaderView::Fixed);
    mastersTable->verticalHeader()->hide();

    pluginsTable->setModel(mFilterProxyModel);
    pluginsTable->setObjectName("PluginsTable");
    pluginsTable->setContextMenuPolicy(Qt::CustomContextMenu);
    pluginsTable->setSortingEnabled(false);
    pluginsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    pluginsTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    pluginsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    pluginsTable->setAlternatingRowColors(true);
    pluginsTable->setVerticalScrollMode(QAbstractItemView::ScrollPerItem);
    pluginsTable->horizontalHeader()->setStretchLastSection(true);
    pluginsTable->horizontalHeader()->hide();

    pluginsTable->verticalHeader()->setDefaultSectionSize(height);
    pluginsTable->verticalHeader()->setResizeMode(QHeaderView::Fixed);

    // Adjust the tableview widths inside the splitter
    QList<int> sizeList;
    sizeList << mLauncherSettings.value(QString("General/MastersTable/width"), QString("200")).toInt();
    sizeList << mLauncherSettings.value(QString("General/PluginTable/width"), QString("340")).toInt();

    splitter->setSizes(sizeList);

    // Create a dialog for the new profile name input
    mNewProfileDialog = new TextInputDialog(tr("New Profile"), tr("Profile name:"), this);

    connect(profilesComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(slotCurrentIndexChanged(int)));

    connect(mNewProfileDialog->lineEdit(), SIGNAL(textChanged(QString)), this, SLOT(updateOkButton(QString)));

    connect(pluginsTable, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(setCheckState(QModelIndex)));
    connect(mastersTable, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(setCheckState(QModelIndex)));

    connect(pluginsTable, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
    connect(mastersTable, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));

    connect(mDataFilesModel, SIGNAL(layoutChanged()), this, SLOT(updateViews()));

    connect(filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterChanged(QString)));

    connect(splitter, SIGNAL(splitterMoved(int,int)), this, SLOT(updateSplitter()));

    createActions();
    setupDataFiles();
}

void DataFilesPage::createActions()
{

    // Add the actions to the toolbuttons
    newProfileButton->setDefaultAction(newProfileAction);
    deleteProfileButton->setDefaultAction(deleteProfileAction);

    // Context menu actions
    mContextMenu = new QMenu(this);
    mContextMenu->addAction(checkAction);
    mContextMenu->addAction(uncheckAction);
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
        profilesComboBox->addItems(profiles);

    // Add the current profile if empty
    if (profilesComboBox->findText(profile) == -1 && !profile.isEmpty())
        profilesComboBox->addItem(profile);

    if (profilesComboBox->findText(QString("Default")) == -1)
        profilesComboBox->addItem(QString("Default"));

    if (profile.isEmpty() || profile == QLatin1String("Default")) {
        deleteProfileAction->setEnabled(false);
        profilesComboBox->setEditEnabled(false);
        profilesComboBox->setCurrentIndex(profilesComboBox->findText(QString("Default")));
    } else {
        profilesComboBox->setEditEnabled(true);
        profilesComboBox->setCurrentIndex(profilesComboBox->findText(profile));
    }

    // We do this here to prevent deletion of profiles when initializing the combobox
    connect(profilesComboBox, SIGNAL(profileRenamed(QString,QString)), this, SLOT(profileRenamed(QString,QString)));
    connect(profilesComboBox, SIGNAL(profileChanged(QString,QString)), this, SLOT(profileChanged(QString,QString)));

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
        profile = profilesComboBox->currentText();
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

void DataFilesPage::updateOkButton(const QString &text)
{
    // We do this here because we need the profiles combobox text
    if (text.isEmpty()) {
         mNewProfileDialog->setOkButtonEnabled(false);
         return;
    }

    (profilesComboBox->findText(text) == -1)
            ? mNewProfileDialog->setOkButtonEnabled(true)
            : mNewProfileDialog->setOkButtonEnabled(false);
}

void DataFilesPage::updateSplitter()
{
    // Sigh, update the saved splitter size in settings only when moved
    // Since getting mSplitter->sizes() if page is hidden returns invalid values
    QList<int> sizes = splitter->sizes();

    mLauncherSettings.setValue(QString("General/MastersTable/width"), QString::number(sizes.at(0)));
    mLauncherSettings.setValue(QString("General/PluginsTable/width"), QString::number(sizes.at(1)));
}

void DataFilesPage::updateViews()
{
    // Ensure the columns are hidden because sort() re-enables them
    mastersTable->setColumnHidden(1, true);
    mastersTable->setColumnHidden(2, true);
    mastersTable->setColumnHidden(3, true);
    mastersTable->setColumnHidden(4, true);
    mastersTable->setColumnHidden(5, true);
    mastersTable->setColumnHidden(6, true);
    mastersTable->setColumnHidden(7, true);
    mastersTable->setColumnHidden(8, true);

    pluginsTable->setColumnHidden(1, true);
    pluginsTable->setColumnHidden(2, true);
    pluginsTable->setColumnHidden(3, true);
    pluginsTable->setColumnHidden(4, true);
    pluginsTable->setColumnHidden(5, true);
    pluginsTable->setColumnHidden(6, true);
    pluginsTable->setColumnHidden(7, true);
    pluginsTable->setColumnHidden(8, true);
}

void DataFilesPage::setProfilesComboBoxIndex(int index)
{
    profilesComboBox->setCurrentIndex(index);
}

void DataFilesPage::slotCurrentIndexChanged(int index)
{
    emit profileChanged(index);
}

QAbstractItemModel* DataFilesPage::profilesComboBoxModel()
{
    return profilesComboBox->model();
}

int DataFilesPage::profilesComboBoxIndex()
{
    return profilesComboBox->currentIndex();
}

void DataFilesPage::on_newProfileAction_triggered()
{
    if (mNewProfileDialog->exec() == QDialog::Accepted) {
        QString profile = mNewProfileDialog->lineEdit()->text();
        profilesComboBox->addItem(profile);
        profilesComboBox->setCurrentIndex(profilesComboBox->findText(profile));
    }
}

void DataFilesPage::on_deleteProfileAction_triggered()
{
    QString profile = profilesComboBox->currentText();

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
        profilesComboBox->removeItem(profilesComboBox->findText(profile));
    }
}

void DataFilesPage::on_checkAction_triggered()
{
    if (pluginsTable->hasFocus())
        setPluginsCheckstates(Qt::Checked);

    if (mastersTable->hasFocus())
        setMastersCheckstates(Qt::Checked);

}

void DataFilesPage::on_uncheckAction_triggered()
{
    if (pluginsTable->hasFocus())
        setPluginsCheckstates(Qt::Unchecked);

    if (mastersTable->hasFocus())
        setMastersCheckstates(Qt::Unchecked);
}

void DataFilesPage::setMastersCheckstates(Qt::CheckState state)
{
    if (!mastersTable->selectionModel()->hasSelection()) {
        return;
    }

    QModelIndexList indexes = mastersTable->selectionModel()->selectedIndexes();

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
    if (!pluginsTable->selectionModel()->hasSelection()) {
        return;
    }

    QModelIndexList indexes = pluginsTable->selectionModel()->selectedIndexes();

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
        deleteProfileAction->setEnabled(false);
        profilesComboBox->setEditEnabled(false);
    } else {
        deleteProfileAction->setEnabled(true);
        profilesComboBox->setEditEnabled(true);
    }

    if (previous.isEmpty())
        return;

    if (profilesComboBox->findText(previous) == -1)
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
    profilesComboBox->removeItem(profilesComboBox->findText(previous));

    loadSettings();

}

void DataFilesPage::showContextMenu(const QPoint &point)
{
    QObject *object = QObject::sender();

    // Not a signal-slot call
    if (!object)
        return;

    if (object->objectName() == QLatin1String("PluginsTable")) {
        if (!pluginsTable->selectionModel()->hasSelection())
            return;

        QPoint globalPos = pluginsTable->mapToGlobal(point);
        QModelIndexList indexes = pluginsTable->selectionModel()->selectedIndexes();

        // Show the check/uncheck actions depending on the state of the selected items
        uncheckAction->setEnabled(false);
        checkAction->setEnabled(false);

        foreach (const QModelIndex &index, indexes)
        {
            if (!index.isValid())
                return;

            QModelIndex sourceIndex = mPluginsProxyModel->mapToSource(
                        mFilterProxyModel->mapToSource(index));

            if (!sourceIndex.isValid())
                return;

            (mDataFilesModel->checkState(sourceIndex) == Qt::Checked)
                    ? uncheckAction->setEnabled(true)
                    : checkAction->setEnabled(true);
        }

        // Show menu
        mContextMenu->exec(globalPos);
    }

    if (object->objectName() == QLatin1String("MastersTable")) {
        if (!mastersTable->selectionModel()->hasSelection())
            return;

        QPoint globalPos = mastersTable->mapToGlobal(point);
        QModelIndexList indexes = mastersTable->selectionModel()->selectedIndexes();

        // Show the check/uncheck actions depending on the state of the selected items
        uncheckAction->setEnabled(false);
        checkAction->setEnabled(false);

        foreach (const QModelIndex &index, indexes)
        {
            if (!index.isValid())
                return;

            QModelIndex sourceIndex = mMastersProxyModel->mapToSource(index);

            if (!sourceIndex.isValid())
                return;

            (mDataFilesModel->checkState(sourceIndex) == Qt::Checked)
                    ? uncheckAction->setEnabled(true)
                    : checkAction->setEnabled(true);
        }

        mContextMenu->exec(globalPos);
    }
}
