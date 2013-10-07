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

#include "components/contentselector/view/contentselector.hpp"

DataFilesPage::DataFilesPage(Files::ConfigurationManager &cfg, GameSettings &gameSettings, LauncherSettings &launcherSettings, QWidget *parent)
    : mCfgMgr(cfg)
    , mGameSettings(gameSettings)
    , mLauncherSettings(launcherSettings)
    , QWidget(parent)
{
    setObjectName ("DataFilesPage");

    unsigned char flags;

    flags = ContentSelectorView::Flag_Content | ContentSelectorView::Flag_Profile;

    ContentSelectorView::ContentSelector::configure(this, flags);
    mSelector = &ContentSelectorView::ContentSelector::instance();

    setupDataFiles();


    connect (mSelector, SIGNAL (signalProfileRenamed (QString, QString)),
                                this, SLOT (slotProfileRenamed (QString, QString)));

    connect (mSelector, SIGNAL (signalProfileChangedByUser (QString, QString)),
                                this, SLOT (slotProfileChangedByUser (QString, QString)));

    connect (mSelector, SIGNAL (signalProfileDeleted (QString)),
                                this, SLOT (slotProfileDeleted (QString)));

    connect (mSelector, SIGNAL (signalAddNewProfile (QString)),
                                this, SLOT (slotAddNewProfile (QString)));
}

void DataFilesPage::loadSettings()
{
    QString profileName = mSelector->getProfileText();

    QStringList files = mLauncherSettings.values(QString("Profiles/") + profileName + QString("/game"), Qt::MatchExactly);
    QStringList addons = mLauncherSettings.values(QString("Profiles/") + profileName + QString("/addon"), Qt::MatchExactly);

    mSelector->clearCheckStates();

    if (files.size() > 0)
        mSelector->setGameFile(files.at(0));
    else
        mSelector->setGameFile();

    mSelector->setCheckStates(addons);
}

void DataFilesPage::saveSettings(const QString &profile)
{
   QString profileName = profile;

   if (profileName.isEmpty())
        profileName = mSelector->getProfileText();

   //retrieve the files selected for the profile
   ContentSelectorModel::ContentFileList items = mSelector->selectedFiles();

   removeProfile (profileName);

    mGameSettings.remove(QString("game"));
    mGameSettings.remove(QString("addon"));

    //set the value of the current profile (not necessarily the profile being saved!)
    mLauncherSettings.setValue(QString("Profiles/currentprofile"), mSelector->getProfileText());

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

void DataFilesPage::removeProfile(const QString &profile)
{
    mLauncherSettings.remove(QString("Profiles/") + profile + QString("/game"));
    mLauncherSettings.remove(QString("Profiles/") + profile + QString("/addon"));
}

void DataFilesPage::changeProfiles(const QString &previous, const QString &current, bool savePrevious)
{
    //abort if no change (typically a duplicate signal)
    if (previous == current)
        return;

    int index = -1;

    if (!previous.isEmpty())
        index = mSelector->getProfileIndex(previous);

    // Store the previous profile if it exists
    if ( (index != -1) && savePrevious)
        saveSettings(previous);

    loadSettings();
}

void DataFilesPage::slotAddNewProfile(const QString &profile)
{
    saveSettings();
    mSelector->clearCheckStates();
    mSelector->addProfile(profile, true);
    mSelector->setGameFile();
    saveSettings();

    emit signalProfileChanged(mSelector->getProfileIndex(profile));
}

void DataFilesPage::slotProfileDeleted (const QString &item)
{
    removeProfile (item);
}

void DataFilesPage::slotProfileChangedByUser(const QString &previous, const QString &current)
{
    changeProfiles(previous, current);
    emit signalProfileChanged(mSelector->getProfileIndex(current));
}

void DataFilesPage::slotProfileRenamed(const QString &previous, const QString &current)
{
    if (previous.isEmpty())
        return;

    // Save the new profile name
    saveSettings();

    // Remove the old one
    removeProfile (previous);

    loadSettings();
}

void DataFilesPage::slotProfileChanged(int index)
{
    mSelector->setProfile(index);
}

void DataFilesPage::setupDataFiles()
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
        mSelector->addProfile (item);

    mSelector->addProfile (profile, true);

    loadSettings();
}

QAbstractItemModel *DataFilesPage::profilesModel() const
{
    return mSelector->profilesModel();
}

int DataFilesPage::profilesIndex() const
{
    return mSelector->getProfileIndex(mSelector->getProfileText());
}
