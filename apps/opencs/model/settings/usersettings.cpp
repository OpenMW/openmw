#include "usersettings.hpp"

#include <QTextStream>
#include <QDir>
#include <QString>
#include <QRegExp>
#include <QMap>
#include <QMessageBox>
#include <QTextCodec>

#include <QFile>
#include <QSortFilterProxyModel>

#include <components/files/configurationmanager.hpp>
#include <boost/version.hpp>

#include "setting.hpp"
#include "support.hpp"

/**
 * Workaround for problems with whitespaces in paths in older versions of Boost library
 */
#if (BOOST_VERSION <= 104600)
namespace boost
{

    template<>
    inline boost::filesystem::path lexical_cast<boost::filesystem::path, std::string>(const std::string& arg)
    {
        return boost::filesystem::path(arg);
    }

} /* namespace boost */
#endif /* (BOOST_VERSION <= 104600) */

CSMSettings::UserSettings *CSMSettings::UserSettings::mUserSettingsInstance = 0;

CSMSettings::UserSettings::UserSettings()
{
    assert(!mUserSettingsInstance);
    mUserSettingsInstance = this;

    buildSettingModelDefaults();
}

void CSMSettings::UserSettings::buildSettingModelDefaults()
{
    QString section = "Window Size";
    {
        Setting *width = createSetting (Type_SingleText, section, "Width");
        Setting *height = createSetting (Type_SingleText, section, "Height");

        width->setWidgetWidth (5);
        height->setWidgetWidth (5);

        width->setDefaultValues (QStringList() << "1024");
        height->setDefaultValues (QStringList() << "768");

        width->setEditorSetting (true);
        height->setEditorSetting (true);

        height->setViewLocation (2,2);
        width->setViewLocation (2,1);

        /*
         *Create the proxy setting for predefined values
         */
        Setting *preDefined = createSetting (Type_SingleList, section,
                                             "Pre-Defined",
                                            QStringList()
                                                << "640 x 480"
                                                << "800 x 600"
                                                << "1024 x 768"
                                                << "1440 x 900"
                                            );

        preDefined->setViewLocation (1, 1);
        preDefined->setWidgetWidth (10);
        preDefined->setColumnSpan (2);

        preDefined->addProxy (width,
                             QStringList() << "640" << "800" << "1024" << "1440"
                             );

        preDefined->addProxy (height,
                             QStringList() << "480" << "600" << "768" << "900"
                             );
    }

    section = "Display Format";
    {
        QString defaultValue = "Icon and Text";

        QStringList values = QStringList()
                            << defaultValue << "Icon Only" << "Text Only";

        Setting *rsd = createSetting (Type_SingleBool,
                                      section, "Record Status Display",
                                      values);

        Setting *ritd = createSetting (Type_SingleBool,
                                      section, "Referenceable ID Type Display",
                                      values);

        rsd->setEditorSetting (true);
        ritd->setEditorSetting (true);
    }

    section = "Proxy Selection Test";
    {
        //create three setting objects, specifying the basic widget type,
        //the setting view name, the page name, and the default value
        Setting *masterBoolean = createSetting (Type_SingleBool, section,
                                        "Master Proxy",
                                        QStringList()
                                            << "Profile One" << "Profile Two"
                                            << "Profile Three" << "Profile Four"
                                );

        Setting *slaveBoolean = createSetting (Type_MultiBool, section,
                                        "Proxy Checkboxes",
                                        QStringList() << "One" << "Two"
                                             << "Three" << "Four" << "Five"
                                );

        Setting *slaveSingleText = createSetting (Type_SingleText, section,
                                                  "Proxy TextBox 1"
                                                );

        Setting *slaveMultiText = createSetting (Type_SingleText, section,
                                                 "ProxyTextBox 2"
                                                 );

        // There are three types of values:
        //
        // Declared values - Pre-determined values, typically for
        // combobox drop downs and boolean (radiobutton / checkbox) labels.
        // These values represent the total possible list of values that may
        // define a setting.  No other values are allowed.
        //
        // Defined values - Values which represent the atual, current value of
        // a setting.  For settings with declared values, this must be one or
        // several declared values, as appropriate.
        //
        // Proxy values - values the proxy master updates the proxy slave when
        // it's own definition is set / changed.  These are definitions for
        // proxy slave settings, but must match any declared values the proxy
        // slave has, if any.

        masterBoolean->addProxy (slaveBoolean, QList <QStringList>()
                                 << (QStringList() << "One" << "Three")
                                 << (QStringList() << "One" << "Three")
                                 << (QStringList() << "One" << "Three" << "Five")
                                 << (QStringList() << "Two" << "Four")
                                 );

        masterBoolean->addProxy (slaveSingleText, QList <QStringList>()
                                 << (QStringList() << "Text A")
                                 << (QStringList() << "Text B")
                                 << (QStringList() << "Text A")
                                 << (QStringList() << "Text C")
                                 );

        masterBoolean->addProxy (slaveMultiText, QList <QStringList>()
                                 << (QStringList() << "One" << "Three")
                                 << (QStringList() << "One" << "Three")
                                 << (QStringList() << "One" << "Three" << "Five")
                                 << (QStringList() << "Two" << "Four")
                                 );

        //settings with proxies are not serialized by default
        //other settings non-serialized for demo purposes
        slaveBoolean->setSerializable (false);
        slaveSingleText->setSerializable (false);
        slaveMultiText->setSerializable (false);

        slaveBoolean->setDefaultValues (QStringList()
                                        << "One" << "Three" << "Five");

        slaveSingleText->setDefaultValue ("Text A");

        slaveMultiText->setDefaultValues (QStringList()
                                         << "One" << "Three" << "Five");

        slaveSingleText->setWidgetWidth (24);
        slaveMultiText->setWidgetWidth (24);
    }
}

CSMSettings::UserSettings::~UserSettings()
{
    mUserSettingsInstance = 0;
}

void CSMSettings::UserSettings::loadSettings (const QString &fileName)
{
    mUserFilePath = QString::fromUtf8
                            (mCfgMgr.getUserConfigPath().c_str()) + fileName.toUtf8();

    QString global = QString::fromUtf8
                                (mCfgMgr.getGlobalPath().c_str()) + fileName.toUtf8();

    QString local = QString::fromUtf8
                                (mCfgMgr.getLocalPath().c_str()) + fileName.toUtf8();

    //open user and global streams
    QTextStream *userStream = openFilestream (mUserFilePath, true);
    QTextStream *otherStream = openFilestream (global, true);

    //failed stream, try for local
    if (!otherStream)
        otherStream = openFilestream (local, true);

    //error condition - notify and return
    if (!otherStream || !userStream)
    {
        QString message = QObject::tr("<br><b>An error was encountered loading \
                user settings files.</b><br><br> One or several files could not \
                be read.  This may be caused by a missing configuration file, \
                incorrect file permissions or a corrupted installation of \
                OpenCS.<br>");

        message += QObject::tr("<br>Global filepath: ") + global;
        message += QObject::tr("<br>Local filepath: ") + local;
        message += QObject::tr("<br>User filepath: ") + mUserFilePath;

        displayFileErrorMessage ( message, true);
        return;
    }

    //success condition - merge the two streams into a single map and save
    DefinitionPageMap totalMap = readFilestream (userStream);
    DefinitionPageMap otherMap = readFilestream(otherStream);

    //merging other settings file in and ignore duplicate settings to
    //avoid overwriting user-level settings
    mergeSettings (totalMap, otherMap);

    if (!totalMap.isEmpty())
        addDefinitions (totalMap);
}

void CSMSettings::UserSettings::saveSettings
                                (const QMap <QString, QStringList> &settingMap)
{
    for (int i = 0; i < settings().size(); i++)
    {
        Setting* setting = settings().at(i);

        QString key = setting->page() + '.' + setting->name();

        if (!settingMap.keys().contains(key))
            continue;

        setting->setDefinedValues (settingMap.value(key));
    }

   writeFilestream (openFilestream (mUserFilePath, false), settingMap);
}

QString CSMSettings::UserSettings::settingValue (const QString &settingKey)
{
    QStringList names = settingKey.split('.');

    Setting *setting = findSetting(names.at(0), names.at(1));

    if (setting)
    {
        if (!setting->definedValues().isEmpty())
            return setting->definedValues().at(0);
    }
    return "";
}

CSMSettings::UserSettings& CSMSettings::UserSettings::instance()
{
    assert(mUserSettingsInstance);
    return *mUserSettingsInstance;
}
