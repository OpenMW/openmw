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
#include "../../view/settings/support.hpp"

#include <QDebug>


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
        Setting *width = new Setting
                                    (Type_SingleText, "Width", section); //, "1024");
        Setting *height = new Setting
                                    (Type_SingleText, "Height", section); //, "768");

        width->setWidgetWidth (5);
        height->setWidgetWidth (5);

        addSetting (width);
        addSetting (height);

        QStringList predefinedValues;

        predefinedValues    << "640 x 480"
                            << "800 x 600"
                            << "1024 x 768"
                            << "1440 x 900";

        Setting *preDefined = new Setting (Type_SingleList, "Pre-Defined",
                                           section); //, "1024 x 768");

        preDefined->setDeclaredValues (predefinedValues);

        //do not serialize since it's values depend entirely on width / height
        preDefined->setSerializable (false);

        QStringList widthValues;
        QStringList heightValues;

        widthValues << "640" << "800" << "1024" << "1440";
        heightValues << "480" << "600" << "768" << "900";

    //    preDefined->addProxy (section, "Width", widthValues);
    //    preDefined->addProxy (section, "Height", heightValues);

        addSetting (preDefined);
    }

    section = "Display Format";
    {
        QString defaultValue = "Icon and Text";

        Setting *rsd = new Setting (Type_SingleBool, "Record Status Display",
                                                        section); //, defaultValue);

        Setting *ritd = new Setting (Type_SingleBool,
                                     "Referenceable ID Type Display",
                                                        section); //, defaultValue);

        QStringList values;

        values << defaultValue << "Icon Only" << "Text Only";

        rsd->setDeclaredValues (values);
        ritd->setDeclaredValues (values);

        addSetting (rsd);
        addSetting (ritd);
    }

    section = "Proxy Selection Test";
    {
        //create three setting objects, specifying the basic widget type,
        //the setting view name, the page name, and the default value
        Setting *masterBoolean = new Setting (Type_SingleBool, "Master Proxy",
                                              section);
        Setting *slaveBoolean = new Setting (Type_MultiBool, "Proxy Checkboxes",
                                             section);
        Setting *slaveSingleText = new Setting (Type_SingleText, "Proxy Textbox",
                                          section);

        Setting *slaveMultiText = new Setting (Type_MultiText, "Proxy Textbox 2",
                                               section);

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

        masterBoolean->setDeclaredValues (QStringList()
                                          << "Profile One" << "Profile Two"
                                          << "Profile Three" << "Profile Four");

        slaveBoolean->setDeclaredValues (QStringList()
                            << "One" << "Two" << "Three" << "Four" << "Five");

        QMap <QString, QStringList> booleanProxyMap;
        QMap <QString, QStringList> textProxyMap;

        booleanProxyMap["Profile One"] = QStringList() << "One" << "Three";
        booleanProxyMap["Profile Two"] = QStringList() << "One" << "Three";
        booleanProxyMap["Profile Three"] =
                                QStringList() << "One" << "Three" << "Five";

        booleanProxyMap["Profile Four"] = QStringList() << "Two" << "Four";

        textProxyMap["Profile One"] = QStringList() << "Text A";
        textProxyMap["Profile Two"] = QStringList() << "Text B";
        textProxyMap["Profile Three"] = QStringList() << "Text A";
        textProxyMap["Profile Four"] = QStringList() << "Text C";

        masterBoolean->addProxy (slaveBoolean, booleanProxyMap);
        masterBoolean->addProxy (slaveSingleText, textProxyMap);
        masterBoolean->addProxy (slaveMultiText, booleanProxyMap);

        //settings with proxies are not serialized by default
        //masterBoolean->setSerializable (false);
        slaveBoolean->setSerializable (false);
        slaveSingleText->setSerializable (false);
        slaveMultiText->setSerializable (false);

        slaveBoolean->setDefaultValues (QStringList() << "One" << "Three" << "Five");
        slaveSingleText->setDefaultValues (QStringList() << "Text A");
        slaveMultiText->setDefaultValues(QStringList() << "One" << "Three" << "Five");

        //add these settings to the model
        addSetting (masterBoolean);
        addSetting (slaveBoolean);
        addSetting (slaveSingleText);
        addSetting (slaveMultiText);
    }
}

CSMSettings::UserSettings::~UserSettings()
{
    mUserSettingsInstance = 0;
}

bool CSMSettings::UserSettings::loadSettingsFromFile
                                                (const QStringList &filepaths)
{
    if (filepaths.isEmpty())
        return false;

    bool success = true;

    DefinitionPageMap totalMap;

    qDebug() << "loading settings";
    foreach (const QString &filepath, filepaths)
    {
        QTextStream *stream = openFilestream (filepath, true);

        DefinitionPageMap pageMap;

        if (stream)
            pageMap = readFilestream(stream);
        else
            success = success && false;

        mergeSettings (totalMap, pageMap, Merge_Overwrite);
    }

    qDebug () << "definitions found:";
    foreach (DefinitionMap *sMap, totalMap)
        foreach (QStringList *stng, *sMap)
            qDebug() << *stng;

    qDebug () << "adding definitions";
    addDefinitions (totalMap);

    return success;
}

void CSMSettings::UserSettings::loadSettings (const QString &fileName)
{
    QStringList filepaths;

    //global
    filepaths << QString::fromStdString
                                (mCfgMgr.getGlobalPath().string()) + fileName;

    //local
    filepaths << QString::fromStdString
                                (mCfgMgr.getLocalPath().string()) + fileName;
    //user
<<<<<<< HEAD

    mUserFilePath = QString::fromStdString(mCfgMgr.getUserPath().string()) + fileName;
    loadSettingsFromFile(mUserFilePath);

    if (!(localOk || globalOk))
=======
    filepaths << QString::fromStdString
                            (mCfgMgr.getUserConfigPath().string()) + fileName;

    qDebug () << "load settings";
    bool success = loadSettingsFromFile (filepaths);

    if (!success)
>>>>>>> esxSelector
    {
        QString message = QObject::tr("<br><b>Could not open user settings \
                files for reading</b><br><br> Global and local settings files \
                could not be read.  You may have incorrect file permissions or \
                the OpenCS installation may be corrupted.<br>");

        message += QObject::tr("<br>Global filepath: ") + filepaths.at(0);
        message += QObject::tr("<br>Local filepath: ") + filepaths.at(1);

        displayFileErrorMessage ( message, true);
    }

    mUserFilePath = filepaths.at (2);
}

void CSMSettings::UserSettings::saveSettings ()
{
   writeFilestream (openFilestream (mUserFilePath, false));
}

CSMSettings::UserSettings& CSMSettings::UserSettings::instance()
{
    assert(mUserSettingsInstance);
    return *mUserSettingsInstance;
}
