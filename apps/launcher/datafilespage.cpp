#include "datafilespage.hpp"
#include "maindialog.hpp"

#include <QClipboard>
#include <QDebug>
#include <QDesktopServices>
#include <QFileDialog>
#include <QList>
#include <QMessageBox>
#include <QPair>
#include <QPushButton>

#include <algorithm>
#include <mutex>
#include <thread>
#include <unordered_set>

#include <apps/launcher/utils/cellnameloader.hpp>

#include <components/files/configurationmanager.hpp>

#include <components/contentselector/model/esmfile.hpp>
#include <components/contentselector/view/contentselector.hpp>

#include <components/config/gamesettings.hpp>
#include <components/config/launchersettings.hpp>

#include <components/bsa/compressedbsafile.hpp>
#include <components/debug/debuglog.hpp>
#include <components/files/qtconversion.hpp>
#include <components/misc/strings/conversion.hpp>
#include <components/navmeshtool/protocol.hpp>
#include <components/settings/values.hpp>
#include <components/vfs/bsaarchive.hpp>
#include <components/vfs/qtconversion.hpp>

#include "utils/profilescombobox.hpp"
#include "utils/textinputdialog.hpp"

#include "ui_directorypicker.h"

const char* Launcher::DataFilesPage::mDefaultContentListName = "Default";

namespace
{
    void contentSubdirs(const QString& path, QStringList& dirs)
    {
        static const QStringList fileFilter{
            "*.esm",
            "*.esp",
            "*.bsa",
            "*.ba2",
            "*.omwgame",
            "*.omwaddon",
            "*.omwscripts",
        };

        static const QStringList dirFilter{
            "animations",
            "bookart",
            "fonts",
            "icons",
            "interface",
            "l10n",
            "meshes",
            "music",
            "mygui",
            "scripts",
            "shaders",
            "sound",
            "splash",
            "strings",
            "textures",
            "trees",
            "video",
        };

        QDir currentDir(path);
        if (!currentDir.entryInfoList(fileFilter, QDir::Files).empty()
            || !currentDir.entryInfoList(dirFilter, QDir::Dirs | QDir::NoDotAndDotDot).empty())
        {
            dirs.push_back(currentDir.canonicalPath());
            return;
        }

        for (const auto& subdir : currentDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot))
            contentSubdirs(subdir.canonicalFilePath(), dirs);
    }

    QList<QPair<int, QListWidgetItem*>> sortedSelectedItems(QListWidget* list, bool reverse = false)
    {
        QList<QPair<int, QListWidgetItem*>> sortedItems;
        for (QListWidgetItem* item : list->selectedItems())
            sortedItems.append(qMakePair(list->row(item), item));

        if (reverse)
            std::sort(sortedItems.begin(), sortedItems.end(), [](auto a, auto b) { return a.first > b.first; });
        else
            std::sort(sortedItems.begin(), sortedItems.end(), [](auto a, auto b) { return a.first < b.first; });

        return sortedItems;
    }
}

namespace Launcher
{
    namespace
    {
        struct HandleNavMeshToolMessage
        {
            int mCellsCount;
            int mExpectedMaxProgress;
            int mMaxProgress;
            int mProgress;

            HandleNavMeshToolMessage operator()(NavMeshTool::ExpectedCells&& message) const
            {
                return HandleNavMeshToolMessage{ static_cast<int>(message.mCount), mExpectedMaxProgress,
                    static_cast<int>(message.mCount) * 100, mProgress };
            }

            HandleNavMeshToolMessage operator()(NavMeshTool::ProcessedCells&& message) const
            {
                return HandleNavMeshToolMessage{ mCellsCount, mExpectedMaxProgress, mMaxProgress,
                    std::max(mProgress, static_cast<int>(message.mCount)) };
            }

            HandleNavMeshToolMessage operator()(NavMeshTool::ExpectedTiles&& message) const
            {
                const int expectedMaxProgress = mCellsCount + static_cast<int>(message.mCount);
                return HandleNavMeshToolMessage{ mCellsCount, expectedMaxProgress,
                    std::max(mMaxProgress, expectedMaxProgress), mProgress };
            }

            HandleNavMeshToolMessage operator()(NavMeshTool::GeneratedTiles&& message) const
            {
                int progress = mCellsCount + static_cast<int>(message.mCount);
                if (mExpectedMaxProgress < mMaxProgress)
                    progress += static_cast<int>(std::round((mMaxProgress - mExpectedMaxProgress)
                        * (static_cast<float>(progress) / static_cast<float>(mExpectedMaxProgress))));
                return HandleNavMeshToolMessage{ mCellsCount, mExpectedMaxProgress, mMaxProgress,
                    std::max(mProgress, progress) };
            }
        };

        int getMaxNavMeshDbFileSizeMiB()
        {
            return Settings::navigator().mMaxNavmeshdbFileSize / (1024 * 1024);
        }
    }
}

Launcher::DataFilesPage::DataFilesPage(const Files::ConfigurationManager& cfg, Config::GameSettings& gameSettings,
    Config::LauncherSettings& launcherSettings, MainDialog* parent)
    : QWidget(parent)
    , mMainDialog(parent)
    , mCfgMgr(cfg)
    , mGameSettings(gameSettings)
    , mLauncherSettings(launcherSettings)
    , mNavMeshToolInvoker(new Process::ProcessInvoker(this))
{
    ui.setupUi(this);
    setObjectName("DataFilesPage");
    mSelector = new ContentSelectorView::ContentSelector(ui.contentSelectorWidget, /*showOMWScripts=*/true);
    const QString encoding = mGameSettings.value("encoding", { "win1252" }).value;
    mSelector->setEncoding(encoding);

    QVector<std::pair<QString, QString>> languages = { { "English", tr("English") }, { "French", tr("French") },
        { "German", tr("German") }, { "Italian", tr("Italian") }, { "Polish", tr("Polish") },
        { "Russian", tr("Russian") }, { "Spanish", tr("Spanish") } };

    for (auto lang : languages)
    {
        mSelector->languageBox()->addItem(lang.second, lang.first);
    }

    mNewProfileDialog = new TextInputDialog(tr("New Content List"), tr("Content List name:"), this);
    mCloneProfileDialog = new TextInputDialog(tr("Clone Content List"), tr("Content List name:"), this);

    connect(mNewProfileDialog->lineEdit(), &LineEdit::textChanged, this, &DataFilesPage::updateNewProfileOkButton);
    connect(mCloneProfileDialog->lineEdit(), &LineEdit::textChanged, this, &DataFilesPage::updateCloneProfileOkButton);
    connect(ui.directoryAddSubdirsButton, &QPushButton::released, this, [this]() { this->addSubdirectories(true); });
    connect(ui.directoryInsertButton, &QPushButton::released, this, [this]() { this->addSubdirectories(false); });
    connect(ui.directoryUpButton, &QPushButton::released, this,
        [this]() { this->moveSources(ui.directoryListWidget, -1); });
    connect(ui.directoryDownButton, &QPushButton::released, this,
        [this]() { this->moveSources(ui.directoryListWidget, 1); });
    connect(ui.directoryRemoveButton, &QPushButton::released, this, &DataFilesPage::removeDirectory);
    connect(
        ui.archiveUpButton, &QPushButton::released, this, [this]() { this->moveSources(ui.archiveListWidget, -1); });
    connect(
        ui.archiveDownButton, &QPushButton::released, this, [this]() { this->moveSources(ui.archiveListWidget, 1); });
    connect(ui.directoryListWidget->model(), &QAbstractItemModel::rowsMoved, this, &DataFilesPage::sortDirectories);
    connect(ui.archiveListWidget->model(), &QAbstractItemModel::rowsMoved, this, &DataFilesPage::sortArchives);

    buildView();
    loadSettings();

    // Connect signal and slot after the settings have been loaded. We only care about the user changing
    // the addons and don't want to get signals of the system doing it during startup.
    connect(mSelector, &ContentSelectorView::ContentSelector::signalAddonDataChanged, this,
        &DataFilesPage::slotAddonDataChanged);
    // Call manually to indicate all changes to addon data during startup.
    slotAddonDataChanged();
}

void Launcher::DataFilesPage::buildView()
{
    QToolButton* refreshButton = mSelector->refreshButton();

    // tool buttons
    ui.newProfileButton->setToolTip("Create a new Content List");
    ui.cloneProfileButton->setToolTip("Clone the current Content List");
    ui.deleteProfileButton->setToolTip("Delete an existing Content List");

    // combo box
    ui.profilesComboBox->addItem(mDefaultContentListName);
    ui.profilesComboBox->setPlaceholderText(QString("Select a Content List..."));
    ui.profilesComboBox->setCurrentIndex(ui.profilesComboBox->findText(QLatin1String(mDefaultContentListName)));

    // Add the actions to the toolbuttons
    ui.newProfileButton->setDefaultAction(ui.newProfileAction);
    ui.cloneProfileButton->setDefaultAction(ui.cloneProfileAction);
    ui.deleteProfileButton->setDefaultAction(ui.deleteProfileAction);
    refreshButton->setDefaultAction(ui.refreshDataFilesAction);

    // establish connections
    connect(ui.profilesComboBox, qOverload<int>(&::ProfilesComboBox::currentIndexChanged), this,
        &DataFilesPage::slotProfileChanged);

    connect(ui.profilesComboBox, &::ProfilesComboBox::profileRenamed, this, &DataFilesPage::slotProfileRenamed);

    connect(ui.profilesComboBox, qOverload<const QString&, const QString&>(&::ProfilesComboBox::signalProfileChanged),
        this, &DataFilesPage::slotProfileChangedByUser);

    connect(ui.refreshDataFilesAction, &QAction::triggered, this, &DataFilesPage::slotRefreshButtonClicked);

    connect(ui.updateNavMeshButton, &QPushButton::clicked, this, &DataFilesPage::startNavMeshTool);
    connect(ui.cancelNavMeshButton, &QPushButton::clicked, this, &DataFilesPage::killNavMeshTool);

    connect(mNavMeshToolInvoker->getProcess(), &QProcess::readyReadStandardOutput, this,
        &DataFilesPage::readNavMeshToolStdout);
    connect(mNavMeshToolInvoker->getProcess(), &QProcess::readyReadStandardError, this,
        &DataFilesPage::readNavMeshToolStderr);
    connect(mNavMeshToolInvoker->getProcess(), qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
        &DataFilesPage::navMeshToolFinished);

    buildArchiveContextMenu();
    buildDataFilesContextMenu();
}

void Launcher::DataFilesPage::slotCopySelectedItemsPaths()
{
    QClipboard* clipboard = QApplication::clipboard();
    QStringList filepaths;

    for (QListWidgetItem* item : ui.directoryListWidget->selectedItems())
    {
        QString path = qvariant_cast<Config::SettingValue>(item->data(Qt::UserRole)).originalRepresentation;
        filepaths.push_back(path);
    }

    if (!filepaths.isEmpty())
    {
        clipboard->setText(filepaths.join("\n"));
    }
}

void Launcher::DataFilesPage::slotOpenSelectedItemsPaths()
{
    QListWidgetItem* item = ui.directoryListWidget->currentItem();
    QUrl confFolderUrl = QUrl::fromLocalFile(qvariant_cast<Config::SettingValue>(item->data(Qt::UserRole)).value);
    QDesktopServices::openUrl(confFolderUrl);
}

void Launcher::DataFilesPage::buildArchiveContextMenu()
{
    connect(ui.archiveListWidget, &QListWidget::customContextMenuRequested, this,
        &DataFilesPage::slotShowArchiveContextMenu);

    mArchiveContextMenu = new QMenu(ui.archiveListWidget);
    mArchiveContextMenu->addAction(tr("&Check Selected"), this, SLOT(slotCheckMultiSelectedItems()));
    mArchiveContextMenu->addAction(tr("&Uncheck Selected"), this, SLOT(slotUncheckMultiSelectedItems()));
}

void Launcher::DataFilesPage::buildDataFilesContextMenu()
{
    connect(ui.directoryListWidget, &QListWidget::customContextMenuRequested, this,
        &DataFilesPage::slotShowDataFilesContextMenu);

    mDataFilesContextMenu = new QMenu(ui.directoryListWidget);
    mDataFilesContextMenu->addAction(
        tr("&Copy Path(s) to Clipboard"), this, &Launcher::DataFilesPage::slotCopySelectedItemsPaths);
    mDataFilesContextMenu->addAction(
        tr("&Open Path in File Explorer"), this, &Launcher::DataFilesPage::slotOpenSelectedItemsPaths);
}

bool Launcher::DataFilesPage::loadSettings()
{
    ui.navMeshMaxSizeSpinBox->setValue(getMaxNavMeshDbFileSizeMiB());

    QStringList profiles = mLauncherSettings.getContentLists();
    QString currentProfile = mLauncherSettings.getCurrentContentListName();

    qDebug() << "The current profile is: " << currentProfile;

    for (const QString& item : profiles)
        addProfile(item, false);

    // Hack: also add the current profile
    if (!currentProfile.isEmpty())
        addProfile(currentProfile, true);

    auto language = mLauncherSettings.getLanguage();

    for (int i = 0; i < mSelector->languageBox()->count(); ++i)
    {
        QString languageItem = mSelector->languageBox()->itemData(i).toString();
        if (language == languageItem)
        {
            mSelector->languageBox()->setCurrentIndex(i);
            break;
        }
    }

    return true;
}

void Launcher::DataFilesPage::populateFileViews(const QString& contentModelName)
{
    mSelector->clearFiles();
    ui.archiveListWidget->clear();
    ui.directoryListWidget->clear();

    QList<Config::SettingValue> directories = mGameSettings.getDataDirs();
    QStringList contentModelDirectories = mLauncherSettings.getDataDirectoryList(contentModelName);
    if (!contentModelDirectories.isEmpty())
    {
        directories.erase(std::remove_if(directories.begin(), directories.end(),
                              [&](const Config::SettingValue& dir) { return mGameSettings.isUserSetting(dir); }),
            directories.end());
        for (const auto& dir : contentModelDirectories)
            directories.push_back(mGameSettings.processPathSettingValue({ dir }));
    }

    mDataLocal = mGameSettings.getDataLocal();
    if (!mDataLocal.isEmpty())
        directories.insert(0, { mDataLocal });

    const auto& resourcesVfs = mGameSettings.getResourcesVfs();
    if (!resourcesVfs.isEmpty())
        directories.insert(0, { resourcesVfs });

    QIcon containsDataIcon(":/images/openmw-plugin.png");

    std::unordered_set<QString> visitedDirectories;
    for (const Config::SettingValue& currentDir : directories)
    {
        if (!visitedDirectories.insert(currentDir.value).second)
            continue;

        // add new achives files presents in current directory
        addArchivesFromDir(currentDir.value);

        QStringList tooltip;

        // add content files presents in current directory
        mSelector->addFiles(currentDir.value, mNewDataDirs.contains(currentDir.value));

        // add current directory to list
        ui.directoryListWidget->addItem(currentDir.originalRepresentation);
        auto row = ui.directoryListWidget->count() - 1;
        auto* item = ui.directoryListWidget->item(row);
        item->setData(Qt::UserRole, QVariant::fromValue(currentDir));

        if (currentDir.value != currentDir.originalRepresentation)
            tooltip << tr("Resolved as %1").arg(currentDir.value);

        // Display new content with custom formatting
        if (mNewDataDirs.contains(currentDir.value))
        {
            tooltip << tr("Will be added to the current profile");
            QFont font = item->font();
            font.setBold(true);
            font.setItalic(true);
            item->setFont(font);
        }

        // deactivate data-local and resources/vfs: they are always included
        // same for ones from non-user config files
        if (currentDir.value == mDataLocal || currentDir.value == resourcesVfs
            || !mGameSettings.isUserSetting(currentDir))
        {
            auto flags = item->flags();
            item->setFlags(flags & ~(Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsEnabled));
            if (currentDir.value == mDataLocal)
                tooltip << tr("This is the data-local directory and cannot be disabled");
            else if (currentDir.value == resourcesVfs)
                tooltip << tr("This directory is part of OpenMW and cannot be disabled");
            else
                tooltip << tr("This directory is enabled in an openmw.cfg other than the user one");
        }

        // Add a "data file" icon if the directory contains a content file
        if (mSelector->containsDataFiles(currentDir.value))
        {
            item->setIcon(containsDataIcon);

            tooltip << tr("Contains content file(s)");
        }
        else
        {
            // Pad to correct vertical alignment
            QPixmap pixmap(QSize(200, 200)); // Arbitrary big number, will be scaled down to widget size
            pixmap.fill(QColor(0, 0, 0, 0));
            auto emptyIcon = QIcon(pixmap);
            item->setIcon(emptyIcon);
        }
        item->setToolTip(tooltip.join('\n'));
    }
    mSelector->sortFiles();

    QList<Config::SettingValue> selectedArchives = mGameSettings.getArchiveList();
    QStringList contentModelSelectedArchives = mLauncherSettings.getArchiveList(contentModelName);
    if (!contentModelSelectedArchives.isEmpty())
    {
        selectedArchives.erase(std::remove_if(selectedArchives.begin(), selectedArchives.end(),
                                   [&](const Config::SettingValue& dir) { return mGameSettings.isUserSetting(dir); }),
            selectedArchives.end());
        for (const auto& dir : contentModelSelectedArchives)
            selectedArchives.push_back({ dir });
    }

    // sort and tick BSA according to profile
    int row = 0;
    for (const auto& archive : selectedArchives)
    {
        const auto match = ui.archiveListWidget->findItems(archive.value, Qt::MatchFixedString);
        if (match.isEmpty())
            continue;
        const auto name = match[0]->text();
        const auto oldrow = ui.archiveListWidget->row(match[0]);
        // entries may be duplicated, e.g. if a content list predated a BSA being added to a non-user config file
        if (oldrow < row)
            continue;
        ui.archiveListWidget->takeItem(oldrow);
        ui.archiveListWidget->insertItem(row, name);
        ui.archiveListWidget->item(row)->setCheckState(Qt::Checked);
        ui.archiveListWidget->item(row)->setData(Qt::UserRole, QVariant::fromValue(archive));
        if (!mGameSettings.isUserSetting(archive))
        {
            auto flags = ui.archiveListWidget->item(row)->flags();
            ui.archiveListWidget->item(row)->setFlags(
                flags & ~(Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsEnabled));
            ui.archiveListWidget->item(row)->setToolTip(
                tr("This archive is enabled in an openmw.cfg other than the user one"));
        }
        row++;
    }

    QStringList nonUserContent;
    for (const auto& content : mGameSettings.getContentList())
    {
        if (!mGameSettings.isUserSetting(content))
            nonUserContent.push_back(content.value);
    }
    mSelector->setNonUserContent(nonUserContent);
    mSelector->setProfileContent(mLauncherSettings.getContentListFiles(contentModelName));
}

void Launcher::DataFilesPage::saveSettings(const QString& profile)
{
    Settings::navigator().mMaxNavmeshdbFileSize.set(
        static_cast<std::uint64_t>(std::max(0, ui.navMeshMaxSizeSpinBox->value())) * 1024 * 1024);

    QString profileName = profile;

    if (profileName.isEmpty())
        profileName = ui.profilesComboBox->currentText();

    // retrieve the data paths
    auto dirList = selectedDirectoriesPaths();

    // retrieve the files selected for the profile
    ContentSelectorModel::ContentFileList items = mSelector->selectedFiles();

    // set the value of the current profile (not necessarily the profile being saved!)
    mLauncherSettings.setCurrentContentListName(ui.profilesComboBox->currentText());

    QStringList fileNames;
    for (const ContentSelectorModel::EsmFile* item : items)
    {
        fileNames.append(item->fileName());
    }
    QStringList dirNames;
    for (const auto& dir : dirList)
    {
        if (mGameSettings.isUserSetting(dir))
            dirNames.push_back(dir.originalRepresentation);
    }
    QStringList archiveNames;
    for (const auto& archive : selectedArchivePaths())
    {
        if (mGameSettings.isUserSetting(archive))
            archiveNames.push_back(archive.originalRepresentation);
    }
    mLauncherSettings.setContentList(profileName, dirNames, archiveNames, fileNames);
    mGameSettings.setContentList(dirList, selectedArchivePaths(), fileNames);

    QString language(mSelector->languageBox()->currentData().toString());

    mLauncherSettings.setLanguage(language);

    if (language == QLatin1String("Polish"))
    {
        mGameSettings.setValue(QLatin1String("encoding"), { "win1250" });
    }
    else if (language == QLatin1String("Russian"))
    {
        mGameSettings.setValue(QLatin1String("encoding"), { "win1251" });
    }
    else
    {
        mGameSettings.setValue(QLatin1String("encoding"), { "win1252" });
    }
}

QList<Config::SettingValue> Launcher::DataFilesPage::selectedDirectoriesPaths() const
{
    QList<Config::SettingValue> dirList;
    for (int i = 0; i < ui.directoryListWidget->count(); ++i)
    {
        const QListWidgetItem* item = ui.directoryListWidget->item(i);
        if (item->flags() & Qt::ItemIsEnabled)
            dirList.append(qvariant_cast<Config::SettingValue>(item->data(Qt::UserRole)));
    }
    return dirList;
}

QList<Config::SettingValue> Launcher::DataFilesPage::selectedArchivePaths() const
{
    QList<Config::SettingValue> archiveList;
    for (int i = 0; i < ui.archiveListWidget->count(); ++i)
    {
        const QListWidgetItem* item = ui.archiveListWidget->item(i);
        if (item->checkState() == Qt::Checked)
            archiveList.append(qvariant_cast<Config::SettingValue>(item->data(Qt::UserRole)));
    }
    return archiveList;
}

QStringList Launcher::DataFilesPage::selectedFilePaths() const
{
    // retrieve the files selected for the profile
    ContentSelectorModel::ContentFileList items = mSelector->selectedFiles();
    QStringList filePaths;
    for (const ContentSelectorModel::EsmFile* item : items)
        if (QFile::exists(item->filePath()))
            filePaths.append(item->filePath());
    return filePaths;
}

void Launcher::DataFilesPage::removeProfile(const QString& profile)
{
    mLauncherSettings.removeContentList(profile);
}

QAbstractItemModel* Launcher::DataFilesPage::profilesModel() const
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

        setProfile(previous, current, savePrevious);
    }
}

void Launcher::DataFilesPage::setProfile(const QString& previous, const QString& current, bool savePrevious)
{
    // abort if no change (poss. duplicate signal)
    if (previous == current)
        return;

    if (!previous.isEmpty() && savePrevious)
        saveSettings(previous);

    ui.profilesComboBox->setCurrentProfile(ui.profilesComboBox->findText(current));

    mNewDataDirs.clear();
    mKnownArchives.clear();
    populateFileViews(current);

    // save list of "old" bsa to be able to display "new" bsa in a different colour
    for (int i = 0; i < ui.archiveListWidget->count(); ++i)
    {
        auto* item = ui.archiveListWidget->item(i);
        mKnownArchives.push_back(item->text());
    }

    checkForDefaultProfile();
}

void Launcher::DataFilesPage::slotProfileDeleted(const QString& item)
{
    removeProfile(item);
}

void Launcher::DataFilesPage::refreshDataFilesView()
{
    QString currentProfile = ui.profilesComboBox->currentText();
    saveSettings(currentProfile);
    populateFileViews(currentProfile);
}

void Launcher::DataFilesPage::slotRefreshButtonClicked()
{
    refreshDataFilesView();
}

void Launcher::DataFilesPage::slotProfileChangedByUser(const QString& previous, const QString& current)
{
    setProfile(previous, current, true);
    emit signalProfileChanged(ui.profilesComboBox->findText(current));
}

void Launcher::DataFilesPage::slotProfileRenamed(const QString& previous, const QString& current)
{
    if (previous.isEmpty())
        return;

    // Save the new profile name
    saveSettings();

    // Remove the old one
    removeProfile(previous);

    loadSettings();
}

void Launcher::DataFilesPage::slotProfileChanged(int index)
{
    // in case the event was triggered externally
    if (ui.profilesComboBox->currentIndex() != index)
        ui.profilesComboBox->setCurrentIndex(index);

    setProfile(index, true);
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

void Launcher::DataFilesPage::addProfile(const QString& profile, bool setAsCurrent)
{
    if (profile.isEmpty())
        return;

    if (ui.profilesComboBox->findText(profile) == -1)
        ui.profilesComboBox->addItem(profile);

    if (setAsCurrent)
        setProfile(ui.profilesComboBox->findText(profile), false);
}

void Launcher::DataFilesPage::on_cloneProfileAction_triggered()
{
    if (mCloneProfileDialog->exec() != QDialog::Accepted)
        return;

    QString profile = mCloneProfileDialog->lineEdit()->text();

    if (profile.isEmpty())
        return;

    const auto& dirList = selectedDirectoriesPaths();
    QStringList dirNames;
    for (const auto& dir : dirList)
    {
        if (mGameSettings.isUserSetting(dir))
            dirNames.push_back(dir.originalRepresentation);
    }
    QStringList archiveNames;
    for (const auto& archive : selectedArchivePaths())
    {
        if (mGameSettings.isUserSetting(archive))
            archiveNames.push_back(archive.originalRepresentation);
    }
    mLauncherSettings.setContentList(profile, dirNames, archiveNames, selectedFilePaths());
    addProfile(profile, true);
}

void Launcher::DataFilesPage::on_deleteProfileAction_triggered()
{
    QString profile = ui.profilesComboBox->currentText();

    if (profile.isEmpty())
        return;

    if (!showDeleteMessageBox(profile))
        return;

    // this should work since the Default profile can't be deleted and is always index 0
    int next = ui.profilesComboBox->currentIndex() - 1;

    // changing the profile forces a reload of plugin file views.
    ui.profilesComboBox->setCurrentIndex(next);

    removeProfile(profile);
    ui.profilesComboBox->removeItem(ui.profilesComboBox->findText(profile));

    checkForDefaultProfile();
}

void Launcher::DataFilesPage::updateNewProfileOkButton(const QString& text)
{
    // We do this here because we need the profiles combobox text
    mNewProfileDialog->setOkButtonEnabled(!text.isEmpty() && ui.profilesComboBox->findText(text) == -1);
}

void Launcher::DataFilesPage::updateCloneProfileOkButton(const QString& text)
{
    // We do this here because we need the profiles combobox text
    mCloneProfileDialog->setOkButtonEnabled(!text.isEmpty() && ui.profilesComboBox->findText(text) == -1);
}

void Launcher::DataFilesPage::addSubdirectories(bool append)
{
    int selectedRow = -1;
    if (append)
    {
        selectedRow = ui.directoryListWidget->count();
    }
    else
    {
        const QList<QPair<int, QListWidgetItem*>> sortedItems = sortedSelectedItems(ui.directoryListWidget);
        if (!sortedItems.isEmpty())
            selectedRow = sortedItems.first().first;
    }

    if (selectedRow == -1)
        return;

    QString rootPath = QFileDialog::getExistingDirectory(
        this, tr("Select Directory"), QDir::homePath(), QFileDialog::ShowDirsOnly | QFileDialog::Option::ReadOnly);

    if (rootPath.isEmpty())
        return;

    const QDir rootDir(rootPath);
    rootPath = rootDir.canonicalPath();

    QStringList subdirs;
    contentSubdirs(rootPath, subdirs);

    // Always offer to append the root directory just in case
    if (subdirs.isEmpty() || subdirs[0] != rootPath)
        subdirs.prepend(rootPath);
    else if (subdirs.size() == 1)
    {
        // We didn't find anything else that looks like a content directory
        // Automatically add the directory selected by user
        if (!ui.directoryListWidget->findItems(rootPath, Qt::MatchFixedString).isEmpty())
            return;
        ui.directoryListWidget->insertItem(selectedRow, rootPath);
        auto* item = ui.directoryListWidget->item(selectedRow);
        item->setData(Qt::UserRole, QVariant::fromValue(Config::SettingValue{ rootPath }));
        mNewDataDirs.push_back(rootPath);
        refreshDataFilesView();
        return;
    }

    QDialog dialog;
    Ui::SelectSubdirs select;

    select.setupUi(&dialog);

    for (const auto& dir : subdirs)
    {
        if (!ui.directoryListWidget->findItems(dir, Qt::MatchFixedString).isEmpty())
            continue;
        const auto lastRow = select.dirListWidget->count();
        select.dirListWidget->addItem(dir);
        select.dirListWidget->item(lastRow)->setCheckState(Qt::Unchecked);
    }

    dialog.show();

    if (dialog.exec() == QDialog::Rejected)
        return;

    for (int i = 0; i < select.dirListWidget->count(); ++i)
    {
        const auto* dir = select.dirListWidget->item(i);
        if (dir->checkState() == Qt::Checked)
        {
            ui.directoryListWidget->insertItem(selectedRow, dir->text());
            auto* item = ui.directoryListWidget->item(selectedRow);
            item->setData(Qt::UserRole, QVariant::fromValue(Config::SettingValue{ dir->text() }));
            mNewDataDirs.push_back(dir->text());
            ++selectedRow;
        }
    }

    refreshDataFilesView();
}

void Launcher::DataFilesPage::sortDirectories()
{
    // Ensure disabled entries (aka default directories) are always at the top.
    for (auto i = 1; i < ui.directoryListWidget->count(); ++i)
    {
        if (!(ui.directoryListWidget->item(i)->flags() & Qt::ItemIsEnabled)
            && (ui.directoryListWidget->item(i - 1)->flags() & Qt::ItemIsEnabled))
        {
            const auto item = ui.directoryListWidget->takeItem(i);
            ui.directoryListWidget->insertItem(i - 1, item);
            ui.directoryListWidget->setCurrentRow(i);
        }
    }
}

void Launcher::DataFilesPage::sortArchives()
{
    // Ensure disabled entries (aka ones from non-user config files) are always at the top.
    for (auto i = 1; i < ui.archiveListWidget->count(); ++i)
    {
        if (!(ui.archiveListWidget->item(i)->flags() & Qt::ItemIsEnabled)
            && (ui.archiveListWidget->item(i - 1)->flags() & Qt::ItemIsEnabled))
        {
            const auto item = ui.archiveListWidget->takeItem(i);
            ui.archiveListWidget->insertItem(i - 1, item);
            ui.archiveListWidget->setCurrentRow(i);
        }
    }
}

void Launcher::DataFilesPage::removeDirectory()
{
    for (const auto& path : ui.directoryListWidget->selectedItems())
        ui.directoryListWidget->takeItem(ui.directoryListWidget->row(path));
    refreshDataFilesView();
}

void Launcher::DataFilesPage::slotShowArchiveContextMenu(const QPoint& pos)
{
    QPoint globalPos = ui.archiveListWidget->viewport()->mapToGlobal(pos);
    mArchiveContextMenu->exec(globalPos);
}

void Launcher::DataFilesPage::slotShowDataFilesContextMenu(const QPoint& pos)
{
    QPoint globalPos = ui.directoryListWidget->viewport()->mapToGlobal(pos);
    mDataFilesContextMenu->exec(globalPos);
}

void Launcher::DataFilesPage::setCheckStateForMultiSelectedItems(bool checked)
{
    Qt::CheckState checkState = checked ? Qt::Checked : Qt::Unchecked;

    for (QListWidgetItem* selectedItem : ui.archiveListWidget->selectedItems())
    {
        selectedItem->setCheckState(checkState);
    }
}

void Launcher::DataFilesPage::slotUncheckMultiSelectedItems()
{
    setCheckStateForMultiSelectedItems(false);
}

void Launcher::DataFilesPage::slotCheckMultiSelectedItems()
{
    setCheckStateForMultiSelectedItems(true);
}

void Launcher::DataFilesPage::moveSources(QListWidget* sourceList, int step)
{
    const QList<QPair<int, QListWidgetItem*>> sortedItems = sortedSelectedItems(sourceList, step > 0);
    for (const auto& i : sortedItems)
    {
        int selectedRow = sourceList->row(i.second);
        int newRow = selectedRow + step;
        if (selectedRow == -1 || newRow < 0 || newRow > sourceList->count() - 1)
            break;

        if (!(sourceList->item(newRow)->flags() & Qt::ItemIsEnabled))
            break;

        const auto item = sourceList->takeItem(selectedRow);
        sourceList->insertItem(newRow, item);
        sourceList->setCurrentRow(newRow);
    }
}

void Launcher::DataFilesPage::addArchive(const QString& name, Qt::CheckState selected, int row)
{
    if (row == -1)
        row = ui.archiveListWidget->count();
    ui.archiveListWidget->insertItem(row, name);
    ui.archiveListWidget->item(row)->setCheckState(selected);
    ui.archiveListWidget->item(row)->setData(Qt::UserRole, QVariant::fromValue(Config::SettingValue{ name }));
    if (mKnownArchives.filter(name).isEmpty()) // XXX why contains doesn't work here ???
    {
        auto item = ui.archiveListWidget->item(row);
        QFont font = item->font();
        font.setBold(true);
        font.setItalic(true);
        item->setFont(font);
    }
}

void Launcher::DataFilesPage::addArchivesFromDir(const QString& path)
{
    QStringList archiveFilter{ "*.bsa", "*.ba2" };
    QDir dir(path);

    std::unordered_set<VFS::Path::Normalized, VFS::Path::Hash> archives;
    for (int i = 0; i < ui.archiveListWidget->count(); ++i)
        archives.insert(VFS::Path::normalizedFromQString(ui.archiveListWidget->item(i)->text()));

    for (const auto& fileinfo : dir.entryInfoList(archiveFilter))
    {
        const auto absPath = fileinfo.absoluteFilePath();
        if (Bsa::BSAFile::detectVersion(Files::pathFromQString(absPath)) == Bsa::BsaVersion::Unknown)
            continue;

        const auto fileName = fileinfo.fileName();

        if (archives.insert(VFS::Path::normalizedFromQString(fileName)).second)
            addArchive(fileName, Qt::Unchecked);
    }
}

void Launcher::DataFilesPage::checkForDefaultProfile()
{
    // don't allow deleting "Default" profile
    bool success = (ui.profilesComboBox->currentText() != mDefaultContentListName);

    ui.deleteProfileAction->setEnabled(success);
    ui.profilesComboBox->setEditEnabled(success);
}

bool Launcher::DataFilesPage::showDeleteMessageBox(const QString& text)
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle(tr("Delete Content List"));
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setStandardButtons(QMessageBox::Cancel);
    msgBox.setText(tr("Are you sure you want to delete <b>%1</b>?").arg(text));

    QAbstractButton* deleteButton = msgBox.addButton(tr("Delete"), QMessageBox::ActionRole);

    msgBox.exec();

    return (msgBox.clickedButton() == deleteButton);
}

void Launcher::DataFilesPage::slotAddonDataChanged()
{
    QStringList selectedFiles = selectedFilePaths();
    if (previousSelectedFiles != selectedFiles)
    {
        previousSelectedFiles = selectedFiles;
        // Loading cells for core Morrowind + Expansions takes about 0.2 seconds, which is enough to cause a
        // barely perceptible UI lag. Splitting into its own thread to alleviate that.
        std::thread loadCellsThread(&DataFilesPage::reloadCells, this, selectedFiles);
        loadCellsThread.detach();
    }
}

// Mutex lock to run reloadCells synchronously.
static std::mutex reloadCellsMutex;

void Launcher::DataFilesPage::reloadCells(QStringList selectedFiles)
{
    // Use a mutex lock so that we can prevent two threads from executing the rest of this code at the same time
    // Based on https://stackoverflow.com/a/5429695/531762
    std::unique_lock<std::mutex> lock(reloadCellsMutex);

    // The following code will run only if there is not another thread currently running it
    CellNameLoader cellNameLoader;
    QSet<QString> set = cellNameLoader.getCellNames(selectedFiles);
    QStringList cellNamesList(set.begin(), set.end());
    std::sort(cellNamesList.begin(), cellNamesList.end());
    emit signalLoadedCellsChanged(cellNamesList);
}

void Launcher::DataFilesPage::startNavMeshTool()
{
    mMainDialog->writeSettings();

    ui.navMeshLogPlainTextEdit->clear();
    ui.navMeshProgressBar->setValue(0);
    ui.navMeshProgressBar->setMaximum(1);
    ui.navMeshProgressBar->resetFormat();

    mNavMeshToolProgress = NavMeshToolProgress{};

    QStringList arguments({ "--write-binary-log" });
    if (ui.navMeshRemoveUnusedTilesCheckBox->checkState() == Qt::Checked)
        arguments.append("--remove-unused-tiles");

    if (!mNavMeshToolInvoker->startProcess(QLatin1String("openmw-navmeshtool"), arguments))
        return;

    ui.cancelNavMeshButton->setEnabled(true);
    ui.navMeshProgressBar->setEnabled(true);
}

void Launcher::DataFilesPage::killNavMeshTool()
{
    mNavMeshToolInvoker->killProcess();
}

void Launcher::DataFilesPage::readNavMeshToolStderr()
{
    updateNavMeshProgress(4096);
}

void Launcher::DataFilesPage::updateNavMeshProgress(int minDataSize)
{
    if (!mNavMeshToolProgress.mEnabled)
        return;
    QProcess& process = *mNavMeshToolInvoker->getProcess();
    mNavMeshToolProgress.mMessagesData.append(process.readAllStandardError());
    if (mNavMeshToolProgress.mMessagesData.size() < minDataSize)
        return;
    const std::byte* const begin = reinterpret_cast<const std::byte*>(mNavMeshToolProgress.mMessagesData.constData());
    const std::byte* const end = begin + mNavMeshToolProgress.mMessagesData.size();
    const std::byte* position = begin;
    HandleNavMeshToolMessage handle{
        mNavMeshToolProgress.mCellsCount,
        mNavMeshToolProgress.mExpectedMaxProgress,
        ui.navMeshProgressBar->maximum(),
        ui.navMeshProgressBar->value(),
    };
    try
    {
        while (true)
        {
            NavMeshTool::Message message;
            const std::byte* const nextPosition = NavMeshTool::deserialize(position, end, message);
            if (nextPosition == position)
                break;
            position = nextPosition;
            handle = std::visit(handle, NavMeshTool::decode(message));
        }
    }
    catch (const std::exception& e)
    {
        Log(Debug::Error) << "Failed to deserialize navmeshtool message: " << e.what();
        mNavMeshToolProgress.mEnabled = false;
        ui.navMeshProgressBar->setFormat("Failed to update progress: " + QString(e.what()));
    }
    if (position != begin)
        mNavMeshToolProgress.mMessagesData = mNavMeshToolProgress.mMessagesData.mid(position - begin);
    mNavMeshToolProgress.mCellsCount = handle.mCellsCount;
    mNavMeshToolProgress.mExpectedMaxProgress = handle.mExpectedMaxProgress;
    ui.navMeshProgressBar->setMaximum(handle.mMaxProgress);
    ui.navMeshProgressBar->setValue(handle.mProgress);
}

void Launcher::DataFilesPage::readNavMeshToolStdout()
{
    QProcess& process = *mNavMeshToolInvoker->getProcess();
    QByteArray& logData = mNavMeshToolProgress.mLogData;
    logData.append(process.readAllStandardOutput());
    const int lineEnd = logData.lastIndexOf('\n');
    if (lineEnd == -1)
        return;
    const int size = logData.size() >= lineEnd && logData[lineEnd - 1] == '\r' ? lineEnd - 1 : lineEnd;
    ui.navMeshLogPlainTextEdit->appendPlainText(QString::fromUtf8(logData.data(), size));
    logData = logData.mid(lineEnd + 1);
}

void Launcher::DataFilesPage::navMeshToolFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    updateNavMeshProgress(0);
    ui.navMeshLogPlainTextEdit->appendPlainText(
        QString::fromUtf8(mNavMeshToolInvoker->getProcess()->readAllStandardOutput()));
    if (exitCode == 0 && exitStatus == QProcess::ExitStatus::NormalExit)
    {
        ui.navMeshProgressBar->setValue(ui.navMeshProgressBar->maximum());
        ui.navMeshProgressBar->resetFormat();
    }
    ui.cancelNavMeshButton->setEnabled(false);
    ui.navMeshProgressBar->setEnabled(false);
}
