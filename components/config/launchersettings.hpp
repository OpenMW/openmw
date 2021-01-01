#ifndef LAUNCHERSETTINGS_HPP
#define LAUNCHERSETTINGS_HPP

#include "settingsbase.hpp"
#include "gamesettings.hpp"

namespace Config
{
    class LauncherSettings : public SettingsBase<QMultiMap<QString, QString> >
    {
    public:
        bool writeFile(QTextStream &stream);

        /// \return names of all Content Lists in the launcher's .cfg file. 
        QStringList getContentLists();

        /// Set initially selected content list to match values from openmw.cfg, creating if necessary
        void setContentList(const GameSettings& gameSettings);

        /// Create a Content List (or replace if it already exists)
        void setContentList(const QString& contentListName, const QStringList& fileNames);

        void removeContentList(const QString &contentListName);

        void setCurrentContentListName(const QString &contentListName);

        QString getCurrentContentListName() const;

        QStringList getContentListFiles(const QString& contentListName) const;

        /// \return new list that is reversed order of input
        static QStringList reverse(const QStringList& toReverse);

        static const char sLauncherConfigFileName[];
    
    private:

        /// \return key to use to get/set the files in the specified Content List
        static QString makeContentListKey(const QString& contentListName);

        /// \return true if both lists are same
        static bool isEqual(const QStringList& list1, const QStringList& list2);

        static QString makeNewContentListName();

        QStringList subKeys(const QString &key);

        /// name of entry in launcher.cfg that holds name of currently selected Content List
        static const char sCurrentContentListKey[];

        /// section of launcher.cfg holding the Content Lists
        static const char sContentListsSectionPrefix[];

        static const char sContentListSuffix[];
    };
}
#endif // LAUNCHERSETTINGS_HPP
