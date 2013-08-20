#include "datafilespage.hpp"

#include <QPushButton>
#include <QMessageBox>
#include <QCheckBox>
#include <QMenu>

#include <components/files/configurationmanager.hpp>

#include <components/esxselector/model/datafilesmodel.hpp>
#include <components/esxselector/model/pluginsproxymodel.hpp>
#include <components/esxselector/model/esmfile.hpp>

#include <components/esxselector/view/lineedit.hpp>
#include <components/esxselector/model/naturalsort.hpp>
#include <components/esxselector/view/profilescombobox.hpp>

#include "components/esxselector/model/masterproxymodel.hpp"
#include "settings/gamesettings.hpp"
#include "settings/launchersettings.hpp"

#include "utils/textinputdialog.hpp"

#include <QDebug>

DataFilesPage::DataFilesPage(Files::ConfigurationManager &cfg, GameSettings &gameSettings, LauncherSettings &launcherSettings, QWidget *parent)
    : mCfgMgr(cfg)
    , mGameSettings(gameSettings)
    , mLauncherSettings(launcherSettings)
    , ContentSelector(parent)
{
    QMetaObject::connectSlotsByName(this);

    projectGroupBox->hide();

    // Create a dialog for the new profile name input
    mNewProfileDialog = new TextInputDialog(tr("New Profile"), tr("Profile name:"), this);

    connect(mNewProfileDialog->lineEdit(), SIGNAL(textChanged(QString)), this, SLOT(updateOkButton(QString)));

    createActions();
    setupDataFiles();
}

void DataFilesPage::createActions()
{
    // Add the actions to the toolbuttons
    newProfileButton->setDefaultAction(newProfileAction);
    deleteProfileButton->setDefaultAction(deleteProfileAction);

    for (int i = 0; i < newProfileButton->actions().size(); i++)
        qDebug() << newProfileButton->actions().at(i)->objectName();
}

void DataFilesPage::setupDataFiles()
{
    if (!mDataFilesModel)
        qDebug() << "data files model undefined";

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

    EsxModel::EsmFileList items = mDataFilesModel->checkedItems();

    foreach(const EsxModel::EsmFile *item, items) {

        if (item->masters().size() == 0) {
            mLauncherSettings.setMultiValue(QString("Profiles/") + profile + QString("/master"), item->fileName());
            mGameSettings.setMultiValue(QString("master"), item->fileName());

        } else {
            mLauncherSettings.setMultiValue(QString("Profiles/") + profile + QString("/plugin"), item->fileName());
            mGameSettings.setMultiValue(QString("plugin"), item->fileName());
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
    if (!pluginView->selectionModel()->hasSelection()) {
        return;
    }

    QModelIndexList indexes = pluginView->selectionModel()->selectedIndexes();

    foreach (const QModelIndex &index, indexes)
    {
        if (!index.isValid())
            return;

        QModelIndex sourceIndex = mPluginsProxyModel->mapToSource(index);

        if (!sourceIndex.isValid())
            return;

        mDataFilesModel->setCheckState(sourceIndex, state);
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
