#ifndef CSMSETTINGS_SETTINGMANAGER_HPP
#define CSMSETTINGS_SETTINGMANAGER_HPP

#include <QObject>
#include <QMap>
#include <QStringList>
#include <QTextStream>

#include "../../view/settings/support.hpp"
#include "setting.hpp"

namespace CSMSettings
{

    typedef QMap <QString, QStringList *> DefinitionMap;
    typedef QMap <QString, DefinitionMap *> DefinitionPageMap;

    typedef QMap <QString, SettingList> SettingPageMap;

    class SettingManager : public QObject
    {
        Q_OBJECT

        QString mReadOnlyMessage;
        QString mReadWriteMessage;
        SettingList mSettings;

    public:
        explicit SettingManager(QObject *parent = 0);

        ///retrieve a setting object from a given page and setting name
        Setting *findSetting
                        (const QString &pageName, const QString &settingName);

        ///retrieve all settings for a specified page
        SettingList findSettings (const QString &pageName);

        ///retrieve all settings named in the attached list.
        ///Setting names are specified in "PageName.SettingName" format.
        SettingList findSettings (const QStringList &list);

        ///Retreive a map of the settings, keyed by page name
        SettingPageMap settingPageMap() const;

    protected:

        ///add a new setting to the model
        void addSetting (Setting *setting);

        ///add definitions to the settings specified in the page map
        void addDefinitions (DefinitionPageMap &pageMap);

        ///read setting definitions from file
        DefinitionPageMap readFilestream(QTextStream *stream);

        ///write setting definitions to file
        bool writeFilestream (QTextStream *stream);

        ///merge PageMaps of settings when loading from multiple files
        void mergeSettings (DefinitionPageMap &destMap, DefinitionPageMap &srcMap,
                            MergeMethod mergeMethod = Merge_Accept);

        QTextStream *openFilestream (const QString &filePath,
                                     bool isReadOnly) const;

        void destroyStream(QTextStream *stream) const;

        void displayFileErrorMessage(const QString &message,
                                     bool isReadOnly) const;

    signals:

    public slots:

    };
}
#endif // CSMSETTINGS_SETTINGMANAGER_HPP
