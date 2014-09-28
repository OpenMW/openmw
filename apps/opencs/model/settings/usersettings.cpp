#include "usersettings.hpp"

#include <QSettings>
#include <QFile>

#include <components/files/configurationmanager.hpp>
#include <components/settings/settings.hpp>
#include <boost/version.hpp>

#include "setting.hpp"
#include "support.hpp"
#include <QTextCodec>
#include <QDebug>

#include <extern/shiny/Main/Factory.hpp>

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

    CSMSettings::UserSettings::UserSettings (const Files::ConfigurationManager& configurationManager)
    : mCfgMgr (configurationManager)
    , mSettingDefinitions(NULL)
    , mSettingCfgDefinitions(NULL)
{
    assert(!mUserSettingsInstance);
    mUserSettingsInstance = this;

    buildSettingModelDefaults();

    // for overriding opencs.ini settings with those from settings.cfg
    mSettingCfgDefinitions = new QSettings(QSettings::IniFormat, QSettings::UserScope, "", QString(), this);
}

void CSMSettings::UserSettings::buildSettingModelDefaults()
{
    QString section;

    section = "Objects";
    {
        Setting *numLights = createSetting (Type_SpinBox, section, "num_lights");
        numLights->setDefaultValue(8);
        numLights->setEditorSetting(true);
        numLights->setColumnSpan (1);
        numLights->setMinimum (2);
        numLights->setMaximum (100); // FIXME: not sure what the max value should be
        numLights->setWidgetWidth (10);
        numLights->setViewLocation(1, 2);

        Setting *shaders = createSetting (Type_CheckBox, section, "shaders");
        shaders->setDeclaredValues(QStringList() << "true" << "false");
        shaders->setDefaultValue("true");
        shaders->setEditorSetting(true);
        shaders->setSpecialValueText("Enable Shaders");
        shaders->setWidgetWidth(25);
        shaders->setColumnSpan (3);
        shaders->setStyleSheet ("QGroupBox { border: 0px; }");
        shaders->setViewLocation(2, 1);
    }

    section = "Scene";
    {
        Setting *fastFactor = createSetting (Type_SpinBox, section, "fast factor");
        fastFactor->setDefaultValue(4);
        fastFactor->setEditorSetting(true);
        fastFactor->setColumnSpan (1);
        fastFactor->setMinimum (1);
        fastFactor->setMaximum (100); // FIXME: not sure what the max value should be
        fastFactor->setWidgetWidth (10);
        fastFactor->setViewLocation(1, 2);

        Setting *farClipDist = createSetting (Type_SpinBox, section, "far clip distance");
        farClipDist->setDefaultValue(300000);
        farClipDist->setEditorSetting(true);
        farClipDist->setColumnSpan (1);
        farClipDist->setMinimum (0);
        farClipDist->setMaximum (1000000); // FIXME: not sure what the max value should be
        farClipDist->setWidgetWidth (10);
        farClipDist->setViewLocation(2, 2);

        Setting *timerStart = createSetting (Type_SpinBox, section, "timer start");
        timerStart->setDefaultValue(20);
        timerStart->setEditorSetting(true);
        timerStart->setColumnSpan (1);
        timerStart->setMinimum (0);
        timerStart->setMaximum (100); // FIXME: not sure what the max value should be
        timerStart->setWidgetWidth (10);
        timerStart->setViewLocation(3, 2);
    }

    section = "SubView";
    {
        Setting *maxSubView = createSetting (Type_SpinBox, section, "max subviews");
        maxSubView->setDefaultValue(256);
        maxSubView->setEditorSetting(true);
        maxSubView->setColumnSpan (1);
        maxSubView->setMinimum (1);
        maxSubView->setMaximum (256); // FIXME: not sure what the max value should be
        maxSubView->setWidgetWidth (10);
        maxSubView->setViewLocation(1, 2);

        Setting *minWidth = createSetting (Type_SpinBox, section, "minimum width");
        minWidth->setDefaultValue(325);
        minWidth->setEditorSetting(true);
        minWidth->setColumnSpan (1);
        minWidth->setMinimum (50);
        minWidth->setMaximum (10000); // FIXME: not sure what the max value should be
        minWidth->setWidgetWidth (10);
        minWidth->setViewLocation(2, 2);

        Setting *reuse = createSetting (Type_CheckBox, section, "reuse");
        reuse->setDeclaredValues(QStringList() << "true" << "false");
        reuse->setDefaultValue("true");
        reuse->setEditorSetting(true);
        reuse->setSpecialValueText("Reuse SubView");
        reuse->setWidgetWidth(25);
        reuse->setColumnSpan (3);
        reuse->setStyleSheet ("QGroupBox { border: 0px; }");
        reuse->setViewLocation(3, 2);
    }

    section = "Window Size";
    {
        Setting *width = createSetting (Type_LineEdit, section, "Width");
        Setting *height = createSetting (Type_LineEdit, section, "Height");

        width->setWidgetWidth (5);
        height->setWidgetWidth (8);

        width->setDefaultValues (QStringList() << "1024");
        height->setDefaultValues (QStringList() << "768");

        width->setEditorSetting (true);
        height->setEditorSetting (true);

        height->setViewLocation (2,2);
        width->setViewLocation (2,1);

        /*
         *Create the proxy setting for predefined values
         */
        Setting *preDefined = createSetting (Type_ComboBox, section,
                                             "Pre-Defined");

        preDefined->setDeclaredValues (QStringList() << "640 x 480"
                                << "800 x 600" << "1024 x 768" << "1440 x 900");

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

        Setting *rsd = createSetting (Type_RadioButton,
                                      section, "Record Status Display");

        Setting *ritd = createSetting (Type_RadioButton,
                                      section, "Referenceable ID Type Display");

        rsd->setDeclaredValues (values);
        ritd->setDeclaredValues (values);

        rsd->setEditorSetting (true);
        ritd->setEditorSetting (true);
    }

    section = "Proxy Selection Test";
    {
        /******************************************************************
        * There are three types of values:
        *
        * Declared values
        *
        *       Pre-determined values, typically for
        *       combobox drop downs and boolean (radiobutton / checkbox) labels.
        *       These values represent the total possible list of values that
        *       may define a setting.  No other values are allowed.
        *
        * Defined values
        *
        *       Values which represent the actual, current value of
        *       a setting.  For settings with declared values, this must be one
        *       or several declared values, as appropriate.
        *
        * Proxy values
        *       Values the proxy master updates the proxy slave when
        *       it's own definition is set / changed.  These are definitions for
        *       proxy slave settings, but must match any declared values the
        *       proxy slave has, if any.
        *******************************************************************/
/*
        //create setting objects, specifying the basic widget type,
        //the page name, and the view name

        Setting *masterBoolean = createSetting (Type_RadioButton, section,
                                                "Master Proxy");

        Setting *slaveBoolean = createSetting (Type_CheckBox, section,
                                                "Proxy Checkboxes");

        Setting *slaveSingleText = createSetting (Type_LineEdit, section,
                                                "Proxy TextBox 1");

        Setting *slaveMultiText = createSetting (Type_LineEdit, section,
                                                "ProxyTextBox 2");

        Setting *slaveAlphaSpinbox = createSetting (Type_SpinBox, section,
                                                "Alpha Spinbox");

        Setting *slaveIntegerSpinbox = createSetting (Type_SpinBox, section,
                                                "Int Spinbox");

        Setting *slaveDoubleSpinbox = createSetting (Type_DoubleSpinBox,
                                                section, "Double Spinbox");

        Setting *slaveSlider = createSetting (Type_Slider, section, "Slider");

        Setting *slaveDial = createSetting (Type_Dial, section, "Dial");

        //set declared values for selected views
        masterBoolean->setDeclaredValues (QStringList()
                                        << "Profile One" << "Profile Two"
                                        << "Profile Three" << "Profile Four");

        slaveBoolean->setDeclaredValues (QStringList()
                            << "One" << "Two" << "Three" << "Four" << "Five");

        slaveAlphaSpinbox->setDeclaredValues (QStringList()
                            << "One" << "Two" << "Three" << "Four");


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

        masterBoolean->addProxy (slaveAlphaSpinbox, QList <QStringList>()
                                 << (QStringList() << "Four")
                                 << (QStringList() << "Three")
                                 << (QStringList() << "Two")
                                 << (QStringList() << "One"));

        masterBoolean->addProxy (slaveIntegerSpinbox, QList <QStringList> ()
                                 << (QStringList() << "0")
                                 << (QStringList() << "7")
                                 << (QStringList() << "14")
                                 << (QStringList() << "21"));

        masterBoolean->addProxy (slaveDoubleSpinbox, QList <QStringList> ()
                                 << (QStringList() << "0.17")
                                 << (QStringList() << "0.34")
                                 << (QStringList() << "0.51")
                                 << (QStringList() << "0.68"));

        masterBoolean->addProxy (slaveSlider, QList <QStringList> ()
                                 << (QStringList() << "25")
                                 << (QStringList() << "50")
                                 << (QStringList() << "75")
                                 << (QStringList() << "100")
                                 );

        masterBoolean->addProxy (slaveDial, QList <QStringList> ()
                                 << (QStringList() << "25")
                                 << (QStringList() << "50")
                                 << (QStringList() << "75")
                                 << (QStringList() << "100")
                                 );

        //settings with proxies are not serialized by default
        //other settings non-serialized for demo purposes
        slaveBoolean->setSerializable (false);
        slaveSingleText->setSerializable (false);
        slaveMultiText->setSerializable (false);
        slaveAlphaSpinbox->setSerializable (false);
        slaveIntegerSpinbox->setSerializable (false);
        slaveDoubleSpinbox->setSerializable (false);
        slaveSlider->setSerializable (false);
        slaveDial->setSerializable (false);

        slaveBoolean->setDefaultValues (QStringList()
                                        << "One" << "Three" << "Five");

        slaveSingleText->setDefaultValue ("Text A");

        slaveMultiText->setDefaultValues (QStringList()
                                         << "One" << "Three" << "Five");

        slaveSingleText->setWidgetWidth (24);
        slaveMultiText->setWidgetWidth (24);

        slaveAlphaSpinbox->setDefaultValue ("Two");
        slaveAlphaSpinbox->setWidgetWidth (20);
        //slaveAlphaSpinbox->setPrefix ("No. ");
        //slaveAlphaSpinbox->setSuffix ("!");
        slaveAlphaSpinbox->setWrapping (true);

        slaveIntegerSpinbox->setDefaultValue (14);
        slaveIntegerSpinbox->setMinimum (0);
        slaveIntegerSpinbox->setMaximum (58);
        slaveIntegerSpinbox->setPrefix ("$");
        slaveIntegerSpinbox->setSuffix (".00");
        slaveIntegerSpinbox->setWidgetWidth (10);
        slaveIntegerSpinbox->setSpecialValueText ("Nothing!");

        slaveDoubleSpinbox->setDefaultValue (0.51);
        slaveDoubleSpinbox->setSingleStep(0.17);
        slaveDoubleSpinbox->setMaximum(4.0);

        slaveSlider->setMinimum (0);
        slaveSlider->setMaximum (100);
        slaveSlider->setDefaultValue (75);
        slaveSlider->setWidgetWidth (100);
        slaveSlider->setTicksAbove (true);
        slaveSlider->setTickInterval (25);

        slaveDial->setMinimum (0);
        slaveDial->setMaximum (100);
        slaveDial->setSingleStep (5);
        slaveDial->setDefaultValue (75);
        slaveDial->setTickInterval (25);
*/
        }
}

CSMSettings::UserSettings::~UserSettings()
{
    mUserSettingsInstance = 0;
}

void CSMSettings::UserSettings::loadSettings (const QString &fileName)
{
    QString userFilePath = QString::fromUtf8
                                (mCfgMgr.getUserConfigPath().string().c_str());

    QString globalFilePath = QString::fromUtf8
                                (mCfgMgr.getGlobalPath().string().c_str());

    QString otherFilePath = globalFilePath;

    //test for local only if global fails (uninstalled copy)
    if (!QFile (globalFilePath + fileName).exists())
    {
        //if global is invalid, use the local path
        otherFilePath = QString::fromUtf8
                                    (mCfgMgr.getLocalPath().string().c_str());
    }

    QSettings::setPath
                (QSettings::IniFormat, QSettings::UserScope, userFilePath);

    QSettings::setPath
                (QSettings::IniFormat, QSettings::SystemScope, otherFilePath);

    mSettingDefinitions = new QSettings
        (QSettings::IniFormat, QSettings::UserScope, "opencs", QString(), this);

    // check if override entry exists (default: disabled)
    if(!mSettingDefinitions->childGroups().contains("Video", Qt::CaseInsensitive))
        mSettingDefinitions->setValue("Video/use settings.cfg", "false");
}

// if the key is not found create one with a defaut value
QString CSMSettings::UserSettings::setting(const QString &viewKey, const QString &value)
{
    if(mSettingDefinitions->contains(viewKey))
        return settingValue(viewKey);
    else if(value != QString())
    {
        mSettingDefinitions->setValue (viewKey, QStringList() << value);
        return value;
    }

    return QString();
}

bool CSMSettings::UserSettings::hasSettingDefinitions (const QString &viewKey) const
{
    return (mSettingDefinitions->contains (viewKey));
}

void CSMSettings::UserSettings::setDefinitions
                                (const QString &key, const QStringList &list)
{
    mSettingDefinitions->setValue (key, list);
}

void CSMSettings::UserSettings::saveDefinitions() const
{
    mSettingDefinitions->sync();
}

QString CSMSettings::UserSettings::settingValue (const QString &settingKey)
{
    QStringList defs;

    // check if video settings are overriden
    if(settingKey.contains(QRegExp("^Video\\b", Qt::CaseInsensitive)) &&
       mSettingDefinitions->value("Video/use settings.cfg") == "true" &&
       settingKey.contains(QRegExp("^Video/\\brender|antialiasing|vsync|fullscreen\\b", Qt::CaseInsensitive)))
    {
        if (!mSettingCfgDefinitions->contains (settingKey))
            return QString();
        else
            defs = mSettingCfgDefinitions->value (settingKey).toStringList();
    }
    else
    {
        if (!mSettingDefinitions->contains (settingKey))
            return QString();

        defs = mSettingDefinitions->value (settingKey).toStringList();
    }

    if (defs.isEmpty())
        return QString();

    return defs.at(0);
}

CSMSettings::UserSettings& CSMSettings::UserSettings::instance()
{
    assert(mUserSettingsInstance);
    return *mUserSettingsInstance;
}

void CSMSettings::UserSettings::updateUserSetting(const QString &settingKey,
                                                    const QStringList &list)
{
    mSettingDefinitions->setValue (settingKey ,list);

    if(settingKey == "Objects/num_lights"
            && mSettingDefinitions->value(settingKey).toString() != list.at(0))
    {
        sh::Factory::getInstance ().setGlobalSetting ("num_lights", list.at(0).toStdString());
    }
    else if(settingKey == "Objects/shaders"
            && mSettingDefinitions->value(settingKey).toString() != list.at(0))
    {
        sh::Factory::getInstance ().setShadersEnabled (list.at(0) == "true" ? true : false);
    }

    emit userSettingUpdated (settingKey, list); // TODO: isn't this circular?
}

CSMSettings::Setting *CSMSettings::UserSettings::findSetting
                        (const QString &pageName, const QString &settingName)
{
    foreach (Setting *setting, mSettings)
    {
        if (setting->name() == settingName)
        {
            if (setting->page() == pageName)
                return setting;
        }
    }
    return 0;
}

void CSMSettings::UserSettings::removeSetting
                        (const QString &pageName, const QString &settingName)
{
    if (mSettings.isEmpty())
        return;

    QList <Setting *>::iterator removeIterator = mSettings.begin();

    while (removeIterator != mSettings.end())
    {
        if ((*removeIterator)->name() == settingName)
        {
            if ((*removeIterator)->page() == pageName)
            {
                mSettings.erase (removeIterator);
                break;
            }
        }
        removeIterator++;
    }
}

CSMSettings::SettingPageMap CSMSettings::UserSettings::settingPageMap() const
{
    SettingPageMap pageMap;

    foreach (Setting *setting, mSettings)
        pageMap[setting->page()].append (setting);

    return pageMap;
}

CSMSettings::Setting *CSMSettings::UserSettings::createSetting
        (CSMSettings::SettingType typ, const QString &page, const QString &name)
{
    //get list of all settings for the current setting name
    if (findSetting (page, name))
    {
        qWarning() << "Duplicate declaration encountered: "
                   << (name + '/' + page);
        return 0;
    }

    Setting *setting = new Setting (typ, name, page);


    //add declaration to the model
    mSettings.append (setting);

    return setting;
}

QStringList CSMSettings::UserSettings::definitions (const QString &viewKey) const
{
    if (mSettingDefinitions->contains (viewKey))
        return mSettingDefinitions->value (viewKey).toStringList();

    return QStringList();
}
