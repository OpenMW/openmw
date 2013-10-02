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

#include <QDebug>

DataFilesPage::DataFilesPage(Files::ConfigurationManager &cfg, GameSettings &gameSettings, LauncherSettings &launcherSettings, QWidget *parent)
    : mCfgMgr(cfg)
    , mGameSettings(gameSettings)
    , mLauncherSettings(launcherSettings)
{
   unsigned char flags;

   flags = ContentSelectorView::Flag_Content | ContentSelectorView::Flag_Profile;

   ContentSelectorView::ContentSelector::configure(this, flags);

    setupDataFiles();

    ContentSelectorView::ContentSelector &cSelector =
            ContentSelectorView::ContentSelector::instance();

    connect (&cSelector, SIGNAL (signalProfileRenamed (QString, QString)),
                                this, SLOT (slotProfileRenamed (QString, QString)));

    connect (&cSelector, SIGNAL (signalProfileChanged (QString, QString)),
                                this, SLOT (slotProfileChanged (QString, QString)));

    connect (&cSelector, SIGNAL (signalProfileDeleted (QString)),
                                this, SLOT (slotProfileDeleted (QString)));

    connect (&cSelector, SIGNAL (signalProfileAdded ()),
                                this, SLOT (slotProfileAdded ()));
}

void DataFilesPage::loadSettings()
{

    QString profile = mLauncherSettings.value(QString("Profiles/currentprofile"));

    if (profile.isEmpty())
        return;

    QStringList files = mLauncherSettings.values(QString("Profiles/") + profile + QString("/master"), Qt::MatchExactly);
    QStringList addons = mLauncherSettings.values(QString("Profiles/") + profile + QString("/plugin"), Qt::MatchExactly);

    foreach (const QString &file, addons)
        files.append(file);

    //ContentSelectorView::ContentSelector::instance().setCheckStates(files);
}

void DataFilesPage::saveSettings()
{
   ContentSelectorModel::ContentFileList items =
           ContentSelectorView::ContentSelector::instance().selectedFiles();

   if (items.size() == 0)
       return;

    QString profile = mLauncherSettings.value(QString("Profiles/currentprofile"));

    if (profile.isEmpty()) {
        profile = ContentSelectorView::ContentSelector::instance().getProfileText();
        mLauncherSettings.setValue(QString("Profiles/currentprofile"), profile);
    }

    mLauncherSettings.remove(QString("Profiles/") + profile + QString("/master"));
    mLauncherSettings.remove(QString("Profiles/") + profile + QString("/plugin"));

    mGameSettings.remove(QString("master"));
    mGameSettings.remove(QString("plugin"));

    foreach(const ContentSelectorModel::EsmFile *item, items) {

        if (item->gameFiles().size() == 0) {
            mLauncherSettings.setMultiValue(QString("Profiles/") + profile + QString("/master"), item->fileName());
            mGameSettings.setMultiValue(QString("master"), item->fileName());

        } else {
            mLauncherSettings.setMultiValue(QString("Profiles/") + profile + QString("/plugin"), item->fileName());
            mGameSettings.setMultiValue(QString("plugin"), item->fileName());
        }
    }

}

void DataFilesPage::slotProfileDeleted (const QString &item)
{
    mLauncherSettings.remove(QString("Profiles/") + item + QString("/master"));
    mLauncherSettings.remove(QString("Profiles/") + item + QString("/plugin"));
}

void DataFilesPage::slotProfileChanged(const QString &previous, const QString &current)
{
    if (previous.isEmpty())
        return;

    if (ContentSelectorView::ContentSelector::instance().getProfileIndex (previous) == -1)
        return; // Profile was deleted

    // Store the previous profile
    mLauncherSettings.setValue(QString("Profiles/currentprofile"), previous);
    saveSettings();
    mLauncherSettings.setValue(QString("Profiles/currentprofile"), current);

    loadSettings();
}

void DataFilesPage::slotProfileRenamed(const QString &previous, const QString &current)
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
    ContentSelectorView::ContentSelector::instance().removeProfile (previous);

    loadSettings();
}

void DataFilesPage::slotProfileAdded()
{
    TextInputDialog newDialog (tr("New Profile"), tr("Profile name:"), this);

     //   connect(mNewDialog->lineEdit(), SIGNAL(textChanged(QString)),
     //           this, SLOT(updateOkButton(QString)));

    if (newDialog.exec() == QDialog::Accepted)
    {
        QString profile = newDialog.lineEdit()->text();

        ContentSelectorView::ContentSelector
                ::instance().addProfile(profile, true);
    }
}

void DataFilesPage::setProfilesComboBoxIndex(int index)
{
    ContentSelectorView::ContentSelector::instance().setProfileIndex(index);
}

void DataFilesPage::setupDataFiles()
{
    ContentSelectorView::ContentSelector &cSelector =
            ContentSelectorView::ContentSelector::instance();

    QStringList paths = mGameSettings.getDataDirs();

    foreach (const QString &path, paths)
        cSelector.addFiles(path);

    QString dataLocal = mGameSettings.getDataLocal();

    if (!dataLocal.isEmpty())
        cSelector.addFiles(dataLocal);

    QStringList profiles = mLauncherSettings.subKeys(QString("Profiles/"));
    QString profile = mLauncherSettings.value(QString("Profiles/currentprofile"));


    foreach (const QString &item, profiles)
        cSelector.addProfile (item);

    cSelector.addProfile (profile, true);

    loadSettings();
}
