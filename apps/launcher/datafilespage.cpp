#include "datafilespage.hpp"

#include <QDebug>

#include <QPushButton>
#include <QMessageBox>
#include <QCheckBox>
#include <QMenu>
#include <QSortFilterProxyModel>
#include <thread>
#include <mutex>

#include <apps/launcher/utils/cellnameloader.hpp>
#include <components/files/configurationmanager.hpp>

#include <components/contentselector/model/esmfile.hpp>
#include <components/contentselector/model/naturalsort.hpp>
#include <components/contentselector/view/contentselector.hpp>

#include <components/config/gamesettings.hpp>
#include <components/config/launchersettings.hpp>
#include <iostream>

#include "utils/textinputdialog.hpp"
#include "utils/profilescombobox.hpp"


const char *Launcher::DataFilesPage::mDefaultContentListName = "Default";

Launcher::DataFilesPage::DataFilesPage(Files::ConfigurationManager &cfg, Config::GameSettings &gameSettings, Config::LauncherSettings &launcherSettings, QWidget *parent)
    : QWidget(parent)
    , mCfgMgr(cfg)
    , mGameSettings(gameSettings)
    , mLauncherSettings(launcherSettings)
{
    ui.setupUi (this);
    setObjectName ("DataFilesPage");
    mSelector = new ContentSelectorView::ContentSelector (ui.contentSelectorWidget);
    const QString encoding = mGameSettings.value("encoding", "win1252");
    mSelector->setEncoding(encoding);

    mNewProfileDialog = new TextInputDialog(tr("New Content List"), tr("Content List name:"), this);
    mCloneProfileDialog = new TextInputDialog(tr("Clone Content List"), tr("Content List name:"), this);

    connect(mNewProfileDialog->lineEdit(), SIGNAL(textChanged(QString)),
            this, SLOT(updateNewProfileOkButton(QString)));
    connect(mCloneProfileDialog->lineEdit(), SIGNAL(textChanged(QString)),
            this, SLOT(updateCloneProfileOkButton(QString)));

    buildView();
    loadSettings();

    // Connect signal and slot after the settings have been loaded. We only care about the user changing
    // the addons and don't want to get signals of the system doing it during startup.
    connect(mSelector, SIGNAL(signalAddonDataChanged(QModelIndex,QModelIndex)),
            this, SLOT(slotAddonDataChanged()));
    // Call manually to indicate all changes to addon data during startup.
    slotAddonDataChanged();
}

void Launcher::DataFilesPage::buildView()
{
    ui.verticalLayout->insertWidget (0, mSelector->uiWidget());

    QToolButton * refreshButton = mSelector->refreshButton();    

    //tool buttons
    ui.newProfileButton->setToolTip ("Create a new Content List");
    ui.cloneProfileButton->setToolTip ("Clone the current Content List");
    ui.deleteProfileButton->setToolTip ("Delete an existing Content List");
    refreshButton->setToolTip("Refresh Data Files");

    //combo box
    ui.profilesComboBox->addItem(mDefaultContentListName);
    ui.profilesComboBox->setPlaceholderText (QString("Select a Content List..."));
    ui.profilesComboBox->setCurrentIndex(ui.profilesComboBox->findText(QLatin1String(mDefaultContentListName)));

    // Add the actions to the toolbuttons
    ui.newProfileButton->setDefaultAction (ui.newProfileAction);
    ui.cloneProfileButton->setDefaultAction (ui.cloneProfileAction);
    ui.deleteProfileButton->setDefaultAction (ui.deleteProfileAction);
    refreshButton->setDefaultAction(ui.refreshDataFilesAction);

    //establish connections
    connect (ui.profilesComboBox, SIGNAL (currentIndexChanged(int)),
             this, SLOT (slotProfileChanged(int)));

    connect (ui.profilesComboBox, SIGNAL (profileRenamed(QString, QString)),
             this, SLOT (slotProfileRenamed(QString, QString)));

    connect (ui.profilesComboBox, SIGNAL (signalProfileChanged(QString, QString)),
             this, SLOT (slotProfileChangedByUser(QString, QString)));

    connect(ui.refreshDataFilesAction, SIGNAL(triggered()),this, SLOT(slotRefreshButtonClicked()));
}

bool Launcher::DataFilesPage::loadSettings()
{
    QStringList profiles = mLauncherSettings.getContentLists();
    QString currentProfile = mLauncherSettings.getCurrentContentListName();

    qDebug() << "The current profile is: " << currentProfile;

    for (const QString &item : profiles)
        addProfile (item, false);

    // Hack: also add the current profile
    if (!currentProfile.isEmpty())
        addProfile(currentProfile, true);

    return true;
}

void Launcher::DataFilesPage::populateFileViews(const QString& contentModelName)
{
    QStringList paths = mGameSettings.getDataDirs();

    mDataLocal = mGameSettings.getDataLocal();

    if (!mDataLocal.isEmpty())
        paths.insert(0, mDataLocal);

    mSelector->clearFiles();

    for (const QString &path : paths)
        mSelector->addFiles(path);

    PathIterator pathIterator(paths);

    mSelector->setProfileContent(filesInProfile(contentModelName, pathIterator));
}

QStringList Launcher::DataFilesPage::filesInProfile(const QString& profileName, PathIterator& pathIterator)
{
    QStringList files = mLauncherSettings.getContentListFiles(profileName);
    QStringList filepaths;

    for (const QString& file : files)
    {
        QString filepath = pathIterator.findFirstPath(file);

        if (!filepath.isEmpty())
            filepaths << filepath;
    }

    return filepaths;
}

void Launcher::DataFilesPage::saveSettings(const QString &profile)
{
   QString profileName = profile;

   if (profileName.isEmpty())
       profileName = ui.profilesComboBox->currentText();

   //retrieve the files selected for the profile
   ContentSelectorModel::ContentFileList items = mSelector->selectedFiles();

    //set the value of the current profile (not necessarily the profile being saved!)
    mLauncherSettings.setCurrentContentListName(ui.profilesComboBox->currentText());

    QStringList fileNames;
    for (const ContentSelectorModel::EsmFile *item : items)
    {
        fileNames.append(item->fileName());
    }
    mLauncherSettings.setContentList(profileName, fileNames);
    mGameSettings.setContentList(fileNames);
}

QStringList Launcher::DataFilesPage::selectedFilePaths()
{
    //retrieve the files selected for the profile
    ContentSelectorModel::ContentFileList items = mSelector->selectedFiles();
    QStringList filePaths;
    for (const ContentSelectorModel::EsmFile *item : items)
    {
        QFile file(item->filePath());
        
        if(file.exists())
        {
            filePaths.append(item->filePath());
        }
        else
        {
            slotRefreshButtonClicked();
        }
    }
    return filePaths;
}

void Launcher::DataFilesPage::removeProfile(const QString &profile)
{
    mLauncherSettings.removeContentList(profile);
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
        QString previous = mPreviousProfile;
        QString current = ui.profilesComboBox->itemText(index);

        mPreviousProfile = current;

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

    ui.profilesComboBox->setCurrentProfile (ui.profilesComboBox->findText (current));

    populateFileViews(current);

    checkForDefaultProfile();
}

void Launcher::DataFilesPage::slotProfileDeleted (const QString &item)
{
    removeProfile (item);
}

void Launcher::DataFilesPage:: refreshDataFilesView ()
{
    QString currentProfile = ui.profilesComboBox->currentText();
    saveSettings(currentProfile);
    populateFileViews(currentProfile);
}

void Launcher::DataFilesPage::slotRefreshButtonClicked ()
{
    refreshDataFilesView();
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
    // in case the event was triggered externally
    if (ui.profilesComboBox->currentIndex() != index)
        ui.profilesComboBox->setCurrentIndex(index);

    setProfile (index, true);
}

void Launcher::DataFilesPage::on_newProfileAction_triggered()
{
    if (mNewProfileDialog->exec() != QDialog::Accepted)
        return;

    QString profile = mNewProfileDialog->lineEdit()->text();

    if (profile.isEmpty())
        return;

    saveSettings();

    mLauncherSettings.setCurrentContentListName(profile);

    addProfile(profile, true);
}

void Launcher::DataFilesPage::addProfile (const QString &profile, bool setAsCurrent)
{
    if (profile.isEmpty())
        return;

    if (ui.profilesComboBox->findText (profile) == -1)
        ui.profilesComboBox->addItem (profile);

    if (setAsCurrent)
        setProfile (ui.profilesComboBox->findText (profile), false);
}

void Launcher::DataFilesPage::on_cloneProfileAction_triggered()
{
    if (mCloneProfileDialog->exec() != QDialog::Accepted)
        return;

    QString profile = mCloneProfileDialog->lineEdit()->text();

    if (profile.isEmpty())
        return;

    mLauncherSettings.setContentList(profile, selectedFilePaths());
    addProfile(profile, true);
}

void Launcher::DataFilesPage::on_deleteProfileAction_triggered()
{
    QString profile = ui.profilesComboBox->currentText();

    if (profile.isEmpty())
        return;

    if (!showDeleteMessageBox (profile))
        return;

    // this should work since the Default profile can't be deleted and is always index 0
    int next = ui.profilesComboBox->currentIndex()-1;

    // changing the profile forces a reload of plugin file views.
    ui.profilesComboBox->setCurrentIndex(next);

    removeProfile(profile);
    ui.profilesComboBox->removeItem(ui.profilesComboBox->findText(profile));

    checkForDefaultProfile();
}

void Launcher::DataFilesPage::updateNewProfileOkButton(const QString &text)
{
    // We do this here because we need the profiles combobox text
    mNewProfileDialog->setOkButtonEnabled(!text.isEmpty() && ui.profilesComboBox->findText(text) == -1);
}

void Launcher::DataFilesPage::updateCloneProfileOkButton(const QString &text)
{
    // We do this here because we need the profiles combobox text
    mCloneProfileDialog->setOkButtonEnabled(!text.isEmpty() && ui.profilesComboBox->findText(text) == -1);
}

void Launcher::DataFilesPage::checkForDefaultProfile()
{
    //don't allow deleting "Default" profile
    bool success = (ui.profilesComboBox->currentText() != mDefaultContentListName);

    ui.deleteProfileAction->setEnabled (success);
    ui.profilesComboBox->setEditEnabled (success);
}

bool Launcher::DataFilesPage::showDeleteMessageBox (const QString &text)
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle(tr("Delete Content List"));
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setStandardButtons(QMessageBox::Cancel);
    msgBox.setText(tr("Are you sure you want to delete <b>%1</b>?").arg(text));

    QAbstractButton *deleteButton =
    msgBox.addButton(tr("Delete"), QMessageBox::ActionRole);

    msgBox.exec();

    return (msgBox.clickedButton() == deleteButton);
}

void Launcher::DataFilesPage::slotAddonDataChanged()
{
    QStringList selectedFiles = selectedFilePaths();
    if (previousSelectedFiles != selectedFiles) {
        previousSelectedFiles = selectedFiles;
        // Loading cells for core Morrowind + Expansions takes about 0.2 seconds, which is enough to cause a
        // barely perceptible UI lag. Splitting into its own thread to alleviate that.
        std::thread loadCellsThread(&DataFilesPage::reloadCells, this, selectedFiles);
        loadCellsThread.detach();
    }
}

// Mutex lock to run reloadCells synchronously.
std::mutex _reloadCellsMutex;

void Launcher::DataFilesPage::reloadCells(QStringList selectedFiles)
{
    // Use a mutex lock so that we can prevent two threads from executing the rest of this code at the same time
    // Based on https://stackoverflow.com/a/5429695/531762
    std::unique_lock<std::mutex> lock(_reloadCellsMutex);

    // The following code will run only if there is not another thread currently running it
    CellNameLoader cellNameLoader;
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
    QSet<QString> set = cellNameLoader.getCellNames(selectedFiles);
    QStringList cellNamesList(set.begin(), set.end());
#else
    QStringList cellNamesList = QStringList::fromSet(cellNameLoader.getCellNames(selectedFiles));
#endif
    std::sort(cellNamesList.begin(), cellNamesList.end());
    emit signalLoadedCellsChanged(cellNamesList);
}
