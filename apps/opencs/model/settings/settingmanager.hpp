#ifndef CSMSETTINGS_SETTINGMANAGER_HPP
#define CSMSETTINGS_SETTINGMANAGER_HPP

#include <QObject>
#include <QMap>
#include <QStringList>
#include <QTextStream>
#include <QSettings>

#include "support.hpp"
#include "setting.hpp"

namespace CSMSettings
{

    typedef QMap <QString, QStringList *> DefinitionMap;
    typedef QMap <QString, DefinitionMap *> DefinitionPageMap;

    typedef QMap <QString, QList <Setting *> > SettingPageMap;

    class SettingManager : public QObject
    {
        Q_OBJECT

        QList <Setting *> mSettings;

    public:
        explicit SettingManager(QObject *parent = 0);

        ///retrieve a setting object from a given page and setting name
        Setting *findSetting
            (const QString &pageName, const QString &settingName = QString());

        ///Retreive a map of the settings, keyed by page name
        SettingPageMap settingPageMap() const;

    protected:

        ///add a new setting to the model and return it
        Setting *createSetting (CSMSettings::SettingType typ,
                            const QString &page, const QString &name);

        ///add definitions to the settings specified in the page map
        void addDefinitions (const QSettings *settings);

    signals:

        void userSettingUpdated (const QString &, const QStringList &);

    public slots:

        void updateUserSetting (const QString &, const QStringList &);
    };
}
#endif // CSMSETTINGS_SETTINGMANAGER_HPP
