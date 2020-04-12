#ifndef DATAFILESPAGE_H
#define DATAFILESPAGE_H

#include "ui_datafilespage.h"
#include <QWidget>


#include <QDir>
#include <QFile>
#include <QStringList>

class QSortFilterProxyModel;
class QAbstractItemModel;
class QMenu;

namespace Files { struct ConfigurationManager; }
namespace ContentSelectorView { class ContentSelector; }
namespace Config { class GameSettings;
                   class LauncherSettings; }

namespace Launcher
{
    class TextInputDialog;
    class ProfilesComboBox;

    class DataFilesPage : public QWidget
    {
        Q_OBJECT

        ContentSelectorView::ContentSelector *mSelector;
        Ui::DataFilesPage ui;

    public:
        explicit DataFilesPage (Files::ConfigurationManager &cfg, Config::GameSettings &gameSettings,
                                Config::LauncherSettings &launcherSettings, QWidget *parent = 0);

        QAbstractItemModel* profilesModel() const;

        int profilesIndex() const;

        //void writeConfig(QString profile = QString());
        void saveSettings(const QString &profile = "");
        bool loadSettings();

        /**
         * Returns the file paths of all selected content files
         * @return the file paths of all selected content files
         */
        QStringList selectedFilePaths();

    signals:
        void signalProfileChanged (int index);
        void signalLoadedCellsChanged(QStringList selectedFiles);

    public slots:
        void slotProfileChanged (int index);

    private slots:

        void slotProfileChangedByUser(const QString &previous, const QString &current);
        void slotProfileRenamed(const QString &previous, const QString &current);
        void slotProfileDeleted(const QString &item);
        void slotAddonDataChanged ();
        void slotRefreshButtonClicked ();

        void updateNewProfileOkButton(const QString &text);
        void updateCloneProfileOkButton(const QString &text);

        void on_newProfileAction_triggered();
        void on_cloneProfileAction_triggered();
        void on_deleteProfileAction_triggered();

    public:
        /// Content List that is always present
        const static char *mDefaultContentListName;

    private:

        TextInputDialog *mNewProfileDialog;
        TextInputDialog *mCloneProfileDialog;

        Files::ConfigurationManager &mCfgMgr;

        Config::GameSettings &mGameSettings;
        Config::LauncherSettings &mLauncherSettings;

        QString mPreviousProfile;
        QStringList previousSelectedFiles;
        QString mDataLocal;

        void setPluginsCheckstates(Qt::CheckState state);

        void buildView();
        void setupConfig();
        void readConfig();
        void setProfile (int index, bool savePrevious);
        void setProfile (const QString &previous, const QString &current, bool savePrevious);
        void removeProfile (const QString &profile);
        bool showDeleteMessageBox (const QString &text);
        void addProfile (const QString &profile, bool setAsCurrent);
        void checkForDefaultProfile();
        void populateFileViews(const QString& contentModelName);
        void reloadCells(QStringList selectedFiles);
        void refreshDataFilesView ();

        class PathIterator
        {
            QStringList::ConstIterator mCitEnd;
            QStringList::ConstIterator mCitCurrent;
            QStringList::ConstIterator mCitBegin;
            QString mFile;
            QString mFilePath;

        public:
            PathIterator (const QStringList &list)
            {
                mCitBegin = list.constBegin();
                mCitCurrent = mCitBegin;
                mCitEnd = list.constEnd();
            }

            QString findFirstPath (const QString &file)
            {
                mCitCurrent = mCitBegin;
                mFile = file;
                return path();
            }

            QString findNextPath () { return path(); }

        private:

            QString path ()
            {
                bool success = false;
                QDir dir;
                QFileInfo file;

                while (!success)
                {
                    if (mCitCurrent == mCitEnd)
                        break;

                    dir.setPath (*(mCitCurrent++));
                    file.setFile (dir.absoluteFilePath (mFile));

                    success = file.exists();
                }

                if (success)
                    return file.absoluteFilePath();

                return "";
            }

        };

        QStringList filesInProfile(const QString& profileName, PathIterator& pathIterator);
    };
}
#endif
