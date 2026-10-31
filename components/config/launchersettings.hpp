#ifndef LAUNCHERSETTINGS_HPP
#define LAUNCHERSETTINGS_HPP

#include <QList>
#include <QMap>
#include <QPair>
#include <QSet>
#include <QString>
#include <QStringList>

#include <map>

class QTextStream;

namespace Config
{
    class GameSettings;

    class LauncherSettings
    {
    public:
        static constexpr char sLauncherConfigFileName[] = "launcher.cfg";
        static constexpr char sLauncherConfigBackupFileName[] = "launcher-wd.dat";

        /// Keys of a section this version does not use, in file order, written back as read
        using UnknownKeys = QList<QPair<QString, QString>>;

        struct Settings
        {
            QString mLanguage;
            bool mWatchdog = false;
            UnknownKeys mUnknown;
        };

        struct MainWindow
        {
            int mWidth = 0;
            int mHeight = 0;
            int mPosX = 0;
            int mPosY = 0;
        };

        struct General
        {
            bool mFirstRun = true;
            MainWindow mMainWindow;
            UnknownKeys mUnknown;
        };

        struct Profile
        {
            QStringList mArchives;
            QStringList mData;
            QStringList mContent;
            QStringList mUserFilesInOrder;
        };

        struct Profiles
        {
            QString mCurrentProfile;
            std::map<QString, Profile> mValues;
            UnknownKeys mUnknown;
        };

        struct Importer
        {
            bool mImportContentSetup = true;
            bool mImportFontSetup = true;
            UnknownKeys mUnknown;
        };

        void readFile(QTextStream& stream, bool missingOnly = false);

        void clear();

        void writeFile(QTextStream& stream) const;

        /// \return names of all Content Lists in the launcher's .cfg file.
        QStringList getContentLists();

        /// Set initially selected content list to match values from openmw.cfg, creating if necessary
        void setContentList(const GameSettings& gameSettings);

        /// Create a Content List (or replace if it already exists)
        void setContentList(const QString& contentListName, const QStringList& dirNames,
            const QStringList& archiveNames, const QStringList& fileNames, const QStringList& userFilesInOrder);

        void removeContentList(const QString& value);

        void setCurrentContentListName(const QString& value) { mProfiles.mCurrentProfile = value; }

        QString getCurrentContentListName() const { return mProfiles.mCurrentProfile; }

        QStringList getDataDirectoryList(const QString& contentListName) const;
        QStringList getUserFilesInOrder(const QString& contentListName) const;
        QStringList getArchiveList(const QString& contentListName) const;
        QStringList getContentListFiles(const QString& contentListName) const;

        bool isFirstRun() const { return mGeneral.mFirstRun; }

        void resetFirstRun() { mGeneral.mFirstRun = false; }

        QString getLanguage() const { return mSettings.mLanguage; }

        void setLanguage(const QString& value) { mSettings.mLanguage = value; }

        bool getWatchdog() const { return mSettings.mWatchdog; }

        MainWindow getMainWindow() const { return mGeneral.mMainWindow; }

        void setMainWindow(const MainWindow& value) { mGeneral.mMainWindow = value; }

        bool getImportContentSetup() const { return mImporter.mImportContentSetup; }

        void setImportContentSetup(bool value) { mImporter.mImportContentSetup = value; }

        bool getImportFontSetup() const { return mImporter.mImportFontSetup; }

        void setImportFontSetup(bool value) { mImporter.mImportFontSetup = value; }

    private:
        Settings mSettings;
        Profiles mProfiles;
        General mGeneral;
        Importer mImporter;
        QSet<QString> mReadKeys;
        /// Sections this version does not know at all, in name order, written back as read
        QMap<QString, UnknownKeys> mUnknownSections;

        bool setValue(const QString& sectionPrefix, const QString& key, const QString& value);

        const Profile* findProfile(const QString& name) const;
    };
}
#endif // LAUNCHERSETTINGS_HPP
