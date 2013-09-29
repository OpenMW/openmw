#include "datafilespage.hpp"

#include <QPushButton>
#include <QMessageBox>
#include <QCheckBox>
#include <QMenu>
#include <QSortFilterProxyModel>

#include <components/files/configurationmanager.hpp>

#include <components/contentselector/model/esmfile.hpp>

#include <components/contentselector/view/lineedit.hpp>
#include <components/contentselector/model/naturalsort.hpp>
#include <components/contentselector/view/profilescombobox.hpp>

#include "settings/gamesettings.hpp"
#include "settings/launchersettings.hpp"

#include "utils/textinputdialog.hpp"
#include "components/contentselector/view/contentselector.hpp"
#include "components/contentselector/model/contentmodel.hpp"

#include <QDebug>

DataFilesPage::DataFilesPage(Files::ConfigurationManager &cfg, GameSettings &gameSettings, LauncherSettings &launcherSettings, QWidget *parent)
    : mCfgMgr(cfg)
    , mGameSettings(gameSettings)
    , mLauncherSettings(launcherSettings)
{
    setupUi(this);
   // mContentSelector.setParent(parent);

   // QMetaObject::connectSlotsByName(this);

    projectGroupBox->hide();

    // Create a dialog for the new profile name input
    mNewProfileDialog = new TextInputDialog(tr("New Profile"), tr("Profile name:"), this);

    connect(mNewProfileDialog->lineEdit(), SIGNAL(textChanged(QString)), this, SLOT(updateOkButton(QString)));



    buildContentModel();
    buildGameFileView();
    buildAddonView();
    buildProfilesView();


    createActions();
    setupDataFiles();


    updateViews();
}

void DataFilesPage::buildContentModel()
{
    mContentModel = new ContentSelectorModel::ContentModel();
    connect(mContentModel, SIGNAL(layoutChanged()), this, SLOT(updateViews()));
}

void DataFilesPage::buildGameFileView()
{
    mGameFileProxyModel = new QSortFilterProxyModel(this);
    mGameFileProxyModel->setFilterRegExp(QString::number((int)ContentSelectorModel::ContentType_GameFile));
    mGameFileProxyModel->setFilterRole (Qt::UserRole);
    mGameFileProxyModel->setSourceModel (mContentModel);

    gameFileView->setPlaceholderText(QString("Select a game file..."));
    gameFileView->setModel(mGameFileProxyModel);

    connect(gameFileView, SIGNAL(currentIndexChanged(int)), this, SLOT(slotCurrentGameFileIndexChanged(int)));

    gameFileView->setCurrentIndex(-1);
    gameFileView->setCurrentIndex(0);
}

void DataFilesPage::buildAddonView()
{
    mAddonProxyModel = new QSortFilterProxyModel(this);
    mAddonProxyModel->setFilterRegExp (QString::number((int)ContentSelectorModel::ContentType_Addon));
    mAddonProxyModel->setFilterRole (Qt::UserRole);
    mAddonProxyModel->setDynamicSortFilter (true);
    mAddonProxyModel->setSourceModel (mContentModel);

    addonView->setModel(mAddonProxyModel);

    connect(addonView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(slotAddonTableItemClicked(const QModelIndex &)));
}

void DataFilesPage::buildProfilesView()
{
    profilesComboBox->setPlaceholderText(QString("Select a profile..."));
    connect(profilesComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(slotCurrentProfileIndexChanged(int)));
}

void DataFilesPage::updateViews()
{
    // Ensure the columns are hidden because sort() re-enables them
    addonView->setColumnHidden(1, true);
    addonView->setColumnHidden(2, true);
    addonView->setColumnHidden(3, true);
    addonView->setColumnHidden(4, true);
    addonView->setColumnHidden(5, true);
    addonView->setColumnHidden(6, true);
    addonView->setColumnHidden(7, true);
    addonView->setColumnHidden(8, true);
    addonView->resizeColumnsToContents();
}

void ContentSelectorView::ContentSelector::addFiles(const QString &path)
{
    mContentModel->addFiles(path);
    //mContentModel->sort(3);  // Sort by date accessed
    gameFileView->setCurrentIndex(-1);
    mContentModel->uncheckAll();
}

void DataFilesPage::createActions()
{
    // Add the actions to the toolbuttons
    newProfileButton->setDefaultAction(newProfileAction);
    deleteProfileButton->setDefaultAction(deleteProfileAction);
}

void DataFilesPage::setupDataFiles()
{
    // Set the encoding to the one found in openmw.cfg or the default
    //mContentSelector.setEncoding(mGameSettings.value(QString("encoding"), QString("win1252")));

    QStringList paths = mGameSettings.getDataDirs();

    foreach (const QString &path, paths) {
        //mContentSelector.
        mContentModel->addFiles(path);
    }

    QString dataLocal = mGameSettings.getDataLocal();
    if (!dataLocal.isEmpty())
        //mContentSelector.
        mContentModel->addFiles(dataLocal);

    // Sort by date accessed for now
    //mContentSelector->sort(3);

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

    gameFileView->setCurrentIndex(-1);
}

void DataFilesPage::loadSettings()
{
    QString profile = mLauncherSettings.value(QString("Profiles/currentprofile"));

    if (profile.isEmpty())
        return;

  //  mContentSelector.
    mContentModel->uncheckAll();

    QStringList gameFiles = mLauncherSettings.values(QString("Profiles/") + profile + QString("/master"), Qt::MatchExactly);
    QStringList addons = mLauncherSettings.values(QString("Profiles/") + profile + QString("/plugin"), Qt::MatchExactly);
}

void DataFilesPage::saveSettings()
{
    if (mContentModel->rowCount() < 1)
        return;

    QString profile = mLauncherSettings.value(QString("Profiles/currentprofile"));

    if (profile.isEmpty()) {
        profile = profilesComboBox->currentText();
        mLauncherSettings.setValue(QString("Profiles/currentprofile"), profile);
    }

    mLauncherSettings.remove(QString("Profiles/") + profile + QString("/master"));
    mLauncherSettings.remove(QString("Profiles/") + profile + QString("/plugin"));

    mGameSettings.remove(QString("master"));
    mGameSettings.remove(QString("plugins"));
    mGameSettings.remove(QString("content"));

    ContentSelectorModel::ContentFileList items = mContentModel->checkedItems();

    foreach(const ContentSelectorModel::EsmFile *item, items) {

        if (item->gameFiles().size() == 0) {
            mLauncherSettings.setMultiValue(QString("Profiles/") + profile + QString("/master"), item->fileName());
            mGameSettings.setMultiValue(QString("content"), item->fileName());

        } else {
            mLauncherSettings.setMultiValue(QString("Profiles/") + profile + QString("/plugin"), item->fileName());
            mGameSettings.setMultiValue(QString("content"), item->fileName());
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

void DataFilesPage::setProfilesComboBoxIndex(int index)
{
    profilesComboBox->setCurrentIndex(index);
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

void DataFilesPage::setPluginsCheckstates(Qt::CheckState state)
{
    if (!addonView->selectionModel()->hasSelection()) {
        return;
    }

    QModelIndexList indexes = addonView->selectionModel()->selectedIndexes();

    foreach (const QModelIndex &index, indexes)
    {
        if (!index.isValid())
            return;

        QModelIndex sourceIndex = mAddonProxyModel->mapToSource(index);

        if (!sourceIndex.isValid())
            return;

        //bool isChecked = ( state == Qt::Checked );

        mContentModel->setData(sourceIndex, state, Qt::CheckStateRole);
    }
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
////////////////////////////

QStringList DataFilesPage::checkedItemsPaths()
{
    QStringList itemPaths;

    foreach( const ContentSelectorModel::EsmFile *file, mContentModel->checkedItems())
        itemPaths << file->path();

    return itemPaths;
}

void DataFilesPage::slotCurrentProfileIndexChanged(int index)
{
    emit profileChanged(index);
}

void DataFilesPage::slotCurrentGameFileIndexChanged(int index)
{
    static int oldIndex = -1;

    QAbstractItemModel *const model = gameFileView->model();
    QSortFilterProxyModel *proxy = dynamic_cast<QSortFilterProxyModel *>(model);

    if (proxy)
        proxy->setDynamicSortFilter(false);

    if (oldIndex > -1)
        model->setData(model->index(oldIndex, 0), false, Qt::UserRole + 1);

    oldIndex = index;

    model->setData(model->index(index, 0), true, Qt::UserRole + 1);

    if (proxy)
        proxy->setDynamicSortFilter(true);
}

void DataFilesPage::slotAddonTableItemClicked(const QModelIndex &index)
{
    QAbstractItemModel *const model = addonView->model();
    //QSortFilterProxyModel *proxy  = dynamic_cast<QSortFilterProxyModel *>(model);

    if (model->data(index, Qt::CheckStateRole).toInt() == Qt::Unchecked)
        model->setData(index, Qt::Checked, Qt::CheckStateRole);
    else
        model->setData(index, Qt::Unchecked, Qt::CheckStateRole);
}
