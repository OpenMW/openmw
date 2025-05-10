#ifndef DATAFILESPAGE_H
#define DATAFILESPAGE_H

#include "ui_datafilespage.h"

#include <components/process/processinvoker.hpp>

#include <QDir>
#include <QMenu>
#include <QStringList>
#include <QWidget>

#include <condition_variable>
#include <mutex>
#include <thread>

class QSortFilterProxyModel;
class QAbstractItemModel;
class QMenu;

namespace Files
{
    struct ConfigurationManager;
}
namespace ContentSelectorView
{
    class ContentSelector;
}
namespace Config
{
    class GameSettings;
    struct SettingValue;
    class LauncherSettings;
}

namespace Launcher
{
    class MainDialog;
    class TextInputDialog;
    class ProfilesComboBox;

    class DataFilesPage : public QWidget
    {
        Q_OBJECT

        ContentSelectorView::ContentSelector* mSelector;
        Ui::DataFilesPage ui;
        QMenu* mArchiveContextMenu;
        QMenu* mDataFilesContextMenu;

    public:
        explicit DataFilesPage(const Files::ConfigurationManager& cfg, Config::GameSettings& gameSettings,
            Config::LauncherSettings& launcherSettings, MainDialog* parent = nullptr);
        ~DataFilesPage();

        QAbstractItemModel* profilesModel() const;

        int profilesIndex() const;

        // void writeConfig(QString profile = QString());
        void saveSettings(const QString& profile = "");
        bool loadSettings();

    signals:
        void signalProfileChanged(int index);
        void signalLoadedCellsChanged(QStringList selectedFiles);

    public slots:
        void slotProfileChanged(int index);

    private slots:

        void slotProfileChangedByUser(const QString& previous, const QString& current);
        void slotProfileRenamed(const QString& previous, const QString& current);
        void slotProfileDeleted(const QString& item);
        void slotAddonDataChanged();
        void slotRefreshButtonClicked();

        void updateNewProfileOkButton(const QString& text);
        void updateCloneProfileOkButton(const QString& text);
        void addSubdirectories(bool append);
        void sortDirectories();
        void sortArchives();
        void removeDirectory();
        void moveSources(QListWidget* sourceList, int step);

        void slotShowArchiveContextMenu(const QPoint& pos);
        void slotShowDataFilesContextMenu(const QPoint& pos);
        void slotCheckMultiSelectedItems();
        void slotUncheckMultiSelectedItems();

        void on_newProfileAction_triggered();
        void on_cloneProfileAction_triggered();
        void on_deleteProfileAction_triggered();

        void startNavMeshTool();
        void killNavMeshTool();
        void readNavMeshToolStdout();
        void readNavMeshToolStderr();
        void navMeshToolFinished(int exitCode, QProcess::ExitStatus exitStatus);

    public:
        /// Content List that is always present
        const static char* mDefaultContentListName;

    private:
        struct NavMeshToolProgress
        {
            bool mEnabled = true;
            QByteArray mLogData;
            QByteArray mMessagesData;
            std::map<std::uint64_t, std::string> mWorldspaces;
            int mCellsCount = 0;
            int mExpectedMaxProgress = 0;
        };

        MainDialog* mMainDialog;
        TextInputDialog* mNewProfileDialog;
        TextInputDialog* mCloneProfileDialog;

        const Files::ConfigurationManager& mCfgMgr;

        Config::GameSettings& mGameSettings;
        Config::LauncherSettings& mLauncherSettings;

        QString mPreviousProfile;
        QStringList mSelectedFiles;
        QString mDataLocal;
        QStringList mKnownArchives;
        QStringList mNewDataDirs;

        Process::ProcessInvoker* mNavMeshToolInvoker;
        NavMeshToolProgress mNavMeshToolProgress;

        bool mReloadCells = false;
        bool mAbortReloadCells = false;
        std::mutex mReloadCellsMutex;
        std::condition_variable mStartReloadCells;
        std::thread mReloadCellsThread;

        void addArchive(const QString& name, Qt::CheckState selected, int row = -1);
        void addArchivesFromDir(const QString& dir);
        void buildView();
        void buildArchiveContextMenu();
        void buildDataFilesContextMenu();
        void setCheckStateForMultiSelectedItems(bool checked);
        void setProfile(int index, bool savePrevious);
        void setProfile(const QString& previous, const QString& current, bool savePrevious);
        void removeProfile(const QString& profile);
        bool showDeleteMessageBox(const QString& text);
        void addProfile(const QString& profile, bool setAsCurrent);
        void checkForDefaultProfile();
        void populateFileViews(const QString& contentModelName);
        void reloadCells();
        void refreshDataFilesView();
        void updateNavMeshProgress(int minDataSize);
        void slotCopySelectedItemsPaths();
        void slotOpenSelectedItemsPaths();

        /**
         * Returns the file paths of all selected content files
         * @return the file paths of all selected content files
         */
        QStringList selectedFilePaths() const;
        QList<Config::SettingValue> selectedArchivePaths() const;
        QList<Config::SettingValue> selectedDirectoriesPaths() const;
    };
}
#endif
