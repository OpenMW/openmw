#include "datafilespage.hpp"

#include <QPushButton>
#include <QMessageBox>
#include <QCheckBox>
#include <QMenu>
#include <QSortFilterProxyModel>
#include <QDebug>

#include <components/files/configurationmanager.hpp>

#include <components/contentselector/model/esmfile.hpp>

#include <components/contentselector/model/naturalsort.hpp>

#include "utils/textinputdialog.hpp"
#include "utils/profilescombobox.hpp"

#include "settings/gamesettings.hpp"
#include "settings/launchersettings.hpp"

#include "components/contentselector/view/contentselector.hpp"

Launcher::DataFilesPage::DataFilesPage(Files::ConfigurationManager &cfg, GameSettings &gameSettings, LauncherSettings &launcherSettings, QWidget *parent)
    : mCfgMgr(cfg)
    , mGameSettings(gameSettings)
    , mLauncherSettings(launcherSettings)
    , QWidget(parent)
{
    ui.setupUi (this);
    setObjectName ("DataFilesPage");
    mSelector = new ContentSelectorView::ContentSelector (ui.contentSelectorWidget);

    buildView();
    setupDataFiles();
}

void Launcher::DataFilesPage::loadSettings()
{
    QString profileName = ui.profilesComboBox->currentText();

    QStringList files = mLauncherSettings.values(QString("Profiles/") + profileName + QString("/game"), Qt::MatchExactly);
    QStringList addons = mLauncherSettings.values(QString("Profiles/") + profileName + QString("/addon"), Qt::MatchExactly);

    mSelector->clearCheckStates();

    QString gameFile ("");

    if (files.size()>0)
        gameFile = files.at (0);

    mSelector->setGameFile(gameFile);
    mSelector->setCheckStates(addons);
}

void Launcher::DataFilesPage::saveSettings(const QString &profile)
{
   QString profileName = profile;

   if (profileName.isEmpty())
       profileName = ui.profilesComboBox->currentText();

   //retrieve the files selected for the profile
   ContentSelectorModel::ContentFileList items = mSelector->selectedFiles();

   removeProfile (profileName);

    mGameSettings.remove(QString("game"));
    mGameSettings.remove(QString("addon"));

    //set the value of the current profile (not necessarily the profile being saved!)
    mLauncherSettings.setValue(QString("Profiles/currentprofile"), ui.profilesComboBox->currentText());

    foreach(const ContentSelectorModel::EsmFile *item, items) {

        if (item->gameFiles().size() == 0) {
            mLauncherSettings.setMultiValue(QString("Profiles/") + profileName + QString("/game"), item->fileName());
            mGameSettings.setMultiValue(QString("game"), item->fileName());
        } else {
            mLauncherSettings.setMultiValue(QString("Profiles/") + profileName + QString("/addon"), item->fileName());
            mGameSettings.setMultiValue(QString("addon"), item->fileName());
        }
    }

}

void Launcher::DataFilesPage::buildView()
{
    ui.verticalLayout->insertWidget (0, mSelector->uiWidget());

    //tool buttons
    ui.newProfileButton->setToolTip ("Create a new profile");
    ui.deleteProfileButton->setToolTip ("Delete an existing profile");

    //combo box
    ui.profilesComboBox->addItem ("Default");
    ui.profilesComboBox->setPlaceholderText (QString("Select a profile..."));

    // Add the actions to the toolbuttons
    ui.newProfileButton->setDefaultAction (ui.newProfileAction);
    ui.deleteProfileButton->setDefaultAction (ui.deleteProfileAction);

    //establish connections
    connect (ui.profilesComboBox, SIGNAL (currentIndexChanged(int)),
             this, SLOT (slotProfileChanged(int)));

    connect (ui.profilesComboBox, SIGNAL (profileRenamed(QString, QString)),
             this, SLOT (slotProfileRenamed(QString, QString)));

    connect (ui.profilesComboBox, SIGNAL (signalProfileChanged(QString, QString)),
             this, SLOT (slotProfileChangedByUser(QString, QString)));
}

void Launcher::DataFilesPage::removeProfile(const QString &profile)
{
    mLauncherSettings.remove(QString("Profiles/") + profile + QString("/game"));
    mLauncherSettings.remove(QString("Profiles/") + profile + QString("/addon"));
}

QAbstractItemModel *Launcher::DataFilesPage::profilesModel() const
{
    return ui.profilesComboBox->model();
}

int Launcher::DataFilesPage::profilesIndex() const
{
    return ui.profilesComboBox->currentIndex();
}

void Launcher::DataFilesPage::setProfile(int index, bool savePrevious)
{
    if (index >= -1 && index < ui.profilesComboBox->count())
    {
        QString previous = ui.profilesComboBox->itemText(ui.profilesComboBox->currentIndex());
        QString current = ui.profilesComboBox->itemText(index);

        setProfile (previous, current, savePrevious);
    }
}

void Launcher::DataFilesPage::setProfile (const QString &previous, const QString &current, bool savePrevious)
{
    //abort if no change (poss. duplicate signal)
    if (previous == current)
            return;

    if (!previous.isEmpty() && savePrevious)
        saveSettings (previous);

    ui.profilesComboBox->setCurrentIndex (ui.profilesComboBox->findText (current));

    loadSettings();

    checkForDefaultProfile();
}

void Launcher::DataFilesPage::slotProfileDeleted (const QString &item)
{
    removeProfile (item);
}

void Launcher::DataFilesPage::slotProfileChangedByUser(const QString &previous, const QString &current)
{
<<<<<<< HEAD
    setProfile(previous, current, true);
    emit signalProfileChanged (ui.profilesComboBox->findText(current));
=======
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

>>>>>>> 3146af34d642a28b15b55f7eb9999d8ac50168a0
}

void Launcher::DataFilesPage::slotProfileRenamed(const QString &previous, const QString &current)
{
    if (previous.isEmpty())
        return;

    // Save the new profile name
    saveSettings();

    // Remove the old one
    removeProfile (previous);

    loadSettings();
}

void Launcher::DataFilesPage::slotProfileChanged(int index)
{
    setProfile (index, true);
}

void Launcher::DataFilesPage::setupDataFiles()
{
    QStringList paths = mGameSettings.getDataDirs();

    foreach (const QString &path, paths)
        mSelector->addFiles(path);

    QString dataLocal = mGameSettings.getDataLocal();

    if (!dataLocal.isEmpty())
        mSelector->addFiles(dataLocal);

    QStringList profiles = mLauncherSettings.subKeys(QString("Profiles/"));
    QString profile = mLauncherSettings.value(QString("Profiles/currentprofile"));

    foreach (const QString &item, profiles)
        addProfile (item, false);

    addProfile (profile, true);

    loadSettings();
}

void Launcher::DataFilesPage::on_newProfileAction_triggered()
{
    TextInputDialog newDialog (tr("New Profile"), tr("Profile name:"), this);

    if (newDialog.exec() != QDialog::Accepted)
        return;

    QString profile = newDialog.getText();

    if (profile.isEmpty())
            return;

    saveSettings();

    mSelector->clearCheckStates();

    addProfile(profile, true);

    mSelector->setGameFile();

    saveSettings();

    emit signalProfileChanged (ui.profilesComboBox->findText(profile));
}

void Launcher::DataFilesPage::addProfile (const QString &profile, bool setAsCurrent)
{
    if (profile.isEmpty())
        return;

    if (ui.profilesComboBox->findText (profile) != -1)
        return;

    ui.profilesComboBox->addItem (profile);

    if (setAsCurrent)
        setProfile (ui.profilesComboBox->findText (profile), false);
}

void Launcher::DataFilesPage::on_deleteProfileAction_triggered()
{
    QString profile = ui.profilesComboBox->currentText();

    if (profile.isEmpty())
        return;

    if (!showDeleteMessageBox (profile))
        return;

    // Remove the profile from the combobox
    ui.profilesComboBox->removeItem (ui.profilesComboBox->findText (profile));

    loadSettings();

    checkForDefaultProfile();
}

void Launcher::DataFilesPage::checkForDefaultProfile()
{
    //don't allow deleting "Default" profile
    bool success = (ui.profilesComboBox->currentText() != "Default");

    ui.deleteProfileAction->setEnabled (success);
    ui.profilesComboBox->setEditEnabled (success);
}

bool Launcher::DataFilesPage::showDeleteMessageBox (const QString &text)
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle(tr("Delete Profile"));
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setStandardButtons(QMessageBox::Cancel);
    msgBox.setText(tr("Are you sure you want to delete <b>%0</b>?").arg(text));

    QAbstractButton *deleteButton =
    msgBox.addButton(tr("Delete"), QMessageBox::ActionRole);

    msgBox.exec();

    return (msgBox.clickedButton() == deleteButton);
}
