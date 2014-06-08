#include "datafilespage.hpp"

#include <QDebug>

#include <QPushButton>
#include <QMessageBox>
#include <QCheckBox>
#include <QMenu>
#include <QSortFilterProxyModel>

#include <components/files/configurationmanager.hpp>

#include <components/contentselector/model/esmfile.hpp>
#include <components/contentselector/model/naturalsort.hpp>
#include <components/contentselector/view/contentselector.hpp>

#include <components/config/gamesettings.hpp>
#include <components/config/launchersettings.hpp>

#include "utils/textinputdialog.hpp"
#include "utils/profilescombobox.hpp"

Launcher::DataFilesPage::DataFilesPage(Files::ConfigurationManager &cfg, Config::GameSettings &gameSettings, Config::LauncherSettings &launcherSettings, QWidget *parent)
    : mCfgMgr(cfg)
    , mGameSettings(gameSettings)
    , mLauncherSettings(launcherSettings)
    , QWidget(parent)
{
    ui.setupUi (this);
    setObjectName ("DataFilesPage");
    mSelector = new ContentSelectorView::ContentSelector (ui.contentSelectorWidget);

    mProfileDialog = new TextInputDialog(tr("New Profile"), tr("Profile name:"), this);

    connect(mProfileDialog->lineEdit(), SIGNAL(textChanged(QString)),
            this, SLOT(updateOkButton(QString)));

    buildView();
    loadSettings();
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
    ui.profilesComboBox->setCurrentIndex(ui.profilesComboBox->findText(QLatin1String("Default")));

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

bool Launcher::DataFilesPage::loadSettings()
{
    QStringList paths = mGameSettings.getDataDirs();

    foreach (const QString &path, paths)
        mSelector->addFiles(path);

    mDataLocal = mGameSettings.getDataLocal();

    if (!mDataLocal.isEmpty())
        mSelector->addFiles(mDataLocal);

    paths.insert (0, mDataLocal);
    PathIterator pathIterator (paths);

    QStringList profiles = mLauncherSettings.subKeys(QString("Profiles/"));
    QString currentProfile = mLauncherSettings.getSettings().value("Profiles/currentprofile");

    qDebug() << "current profile is: " << currentProfile;

    foreach (const QString &item, profiles)
        addProfile (item, false);

    // Hack: also add the current profile
    if (!currentProfile.isEmpty())
        addProfile(currentProfile, true);

    QStringList files = mLauncherSettings.values(QString("Profiles/") + currentProfile + QString("/content"), Qt::MatchExactly);
    QStringList filepaths;

    foreach (const QString &file, files)
    {
        QString filepath = pathIterator.findFirstPath (file);

        if (!filepath.isEmpty())
            filepaths << filepath;
    }

    mSelector->setProfileContent (filepaths);

    return true;
}

void Launcher::DataFilesPage::saveSettings(const QString &profile)
{
   QString profileName = profile;

   if (profileName.isEmpty())
       profileName = ui.profilesComboBox->currentText();

   //retrieve the files selected for the profile
   ContentSelectorModel::ContentFileList items = mSelector->selectedFiles();

   removeProfile (profileName);

    mGameSettings.remove(QString("content"));

    //set the value of the current profile (not necessarily the profile being saved!)
    mLauncherSettings.setValue(QString("Profiles/currentprofile"), ui.profilesComboBox->currentText());

    foreach(const ContentSelectorModel::EsmFile *item, items) {
        mLauncherSettings.setMultiValue(QString("Profiles/") + profileName + QString("/content"), item->fileName());
        mGameSettings.setMultiValue(QString("content"), item->fileName());
    }

}

void Launcher::DataFilesPage::removeProfile(const QString &profile)
{
    mLauncherSettings.remove(QString("Profiles/") + profile);
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

    if (previous.isEmpty())
           return;

    if (!previous.isEmpty() && savePrevious)
        saveSettings (previous);

    ui.profilesComboBox->setCurrentProfile (ui.profilesComboBox->findText (current));

    loadSettings();

    checkForDefaultProfile();
}

void Launcher::DataFilesPage::slotProfileDeleted (const QString &item)
{
    removeProfile (item);
}

void Launcher::DataFilesPage::slotProfileChangedByUser(const QString &previous, const QString &current)
{
    setProfile(previous, current, true);
    emit signalProfileChanged (ui.profilesComboBox->findText(current));
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

void Launcher::DataFilesPage::on_newProfileAction_triggered()
{
    if (!mProfileDialog->exec() == QDialog::Accepted)
        return;

    QString profile = mProfileDialog->lineEdit()->text();

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

    removeProfile(profile);

    saveSettings();

    loadSettings();

    checkForDefaultProfile();
}

void Launcher::DataFilesPage::updateOkButton(const QString &text)
{
    // We do this here because we need the profiles combobox text
    if (text.isEmpty()) {
         mProfileDialog->setOkButtonEnabled(false);
         return;
    }

    (ui.profilesComboBox->findText(text) == -1)
            ? mProfileDialog->setOkButtonEnabled(true)
            : mProfileDialog->setOkButtonEnabled(false);
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
