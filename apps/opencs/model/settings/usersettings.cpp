#include "usersettings.hpp"

#include <QSettings>
#include <QFile>

#include <components/files/configurationmanager.hpp>
#include <components/settings/settings.hpp>
#include <components/contentselector/model/naturalsort.hpp>

#include <boost/version.hpp>

#include <OgreRoot.h>

#include "setting.hpp"
#include "support.hpp"
#include <QTextCodec>
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

namespace CSMSettings
{

UserSettings *UserSettings::mUserSettingsInstance = 0;

UserSettings::UserSettings (const Files::ConfigurationManager& configurationManager)
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

void UserSettings::buildSettingModelDefaults()
{
    QString page;

    page = "Shader";
    {
        QString section = "Group1";

        Setting *numLights = createSetting (Type_SpinBox, page, "num lights");
        numLights->setDefaultValue(8);
        numLights->setEditorSetting(true);
        numLights->setColumnSpan (1);
        numLights->setMinimum (0);
        numLights->setMaximum (100); // FIXME: not sure what the max value should be
        numLights->setWidgetWidth (10);
        numLights->setSpecialValueText ("Nothing!"); // text to display when value is 0
        numLights->setViewLocation(1, 2);
        Setting *nlText = createSetting (Type_Undefined, page, "nlText");
        nlText->setSpecialValueText("Num Lights"); // hack to place text labels
        nlText->setEditorSetting(false);
        nlText->setSerializable (false);
        nlText->setColumnSpan (1);
        nlText->setWidgetWidth (10);
        nlText->setViewLocation(1, 1);


        Setting *simpleWater = createSetting (Type_CheckBox, page, "simple_water");
        simpleWater->setDeclaredValues(QStringList() << "true" << "false");
        simpleWater->setDefaultValue("false");
        simpleWater->setEditorSetting(true);
        simpleWater->setSpecialValueText("Enable Simple Water");
        simpleWater->setWidgetWidth(25);
        simpleWater->setColumnSpan (3);
        simpleWater->setStyleSheet ("QGroupBox { border: 0px; }");
        simpleWater->setViewLocation(1, 5);

        Setting *waterEnabled = createSetting (Type_DoubleSpinBox, page, "waterEnabled");
        waterEnabled->setDefaultValue(0.00);
        waterEnabled->setEditorSetting(true);
        waterEnabled->setColumnSpan (1);
        waterEnabled->setMinimum (0);
        waterEnabled->setMaximum (100.00); // FIXME: not sure what the max value should be
        waterEnabled->setWidgetWidth (10);
        waterEnabled->setViewLocation(2, 6);
        Setting *weText = createSetting (Type_Undefined, page, "weText");
        weText->setSpecialValueText("Water Enabled");
        weText->setEditorSetting(false);
        weText->setSerializable (false);
        weText->setColumnSpan (1);
        weText->setWidgetWidth (10);
        weText->setViewLocation(2, 5);

        Setting *waterLevel = createSetting (Type_DoubleSpinBox, page, "waterLevel");
        waterLevel->setDefaultValue(0.00);
        waterLevel->setEditorSetting(true);
        waterLevel->setColumnSpan (1);
        waterLevel->setMinimum (0);
        waterLevel->setMaximum (100.00); // FIXME: not sure what the max value should be
        waterLevel->setWidgetWidth (10);
        waterLevel->setViewLocation(3, 6);
        Setting *wlText = createSetting (Type_Undefined, page, "wlText");
        wlText->setSpecialValueText("Water Level");
        wlText->setEditorSetting(false);
        wlText->setSerializable (false);
        wlText->setColumnSpan (1);
        wlText->setWidgetWidth (10);
        wlText->setViewLocation(3, 5);

        Setting *waterTimer = createSetting (Type_DoubleSpinBox, page, "waterTimer");
        waterTimer->setDefaultValue(0.00);
        waterTimer->setEditorSetting(true);
        waterTimer->setColumnSpan (1);
        waterTimer->setMinimum (0);
        waterTimer->setMaximum (100.00); // FIXME: not sure what the max value should be
        waterTimer->setWidgetWidth (10);
        waterTimer->setViewLocation(4, 6);
        Setting *wtText = createSetting (Type_Undefined, page, "wtText");
        wtText->setSpecialValueText("Water Timer");
        wtText->setEditorSetting(false);
        wtText->setSerializable (false);
        wtText->setColumnSpan (1);
        wtText->setWidgetWidth (10);
        wtText->setViewLocation(4, 5);


        Setting *spaceText = createSetting (Type_Undefined, page, "spaceText");
        spaceText->setSpecialValueText(" ");
        spaceText->setEditorSetting(false);
        spaceText->setSerializable (false);
        spaceText->setColumnSpan (1);
        spaceText->setWidgetWidth (5);
        spaceText->setViewLocation(3, 4);
        Setting *spaceText2 = createSetting (Type_Undefined, page, "spaceText2");
        spaceText2->setSpecialValueText(" ");
        spaceText2->setEditorSetting(false);
        spaceText2->setSerializable (false);
        spaceText2->setColumnSpan (1);
        spaceText2->setWidgetWidth (5);
        spaceText2->setViewLocation(3, 7);

#if 0
sh::Factory::getInstance ().setSharedParameter ("windDir_windSpeed", sh::makeProperty<sh::Vector3>(new sh::Vector3(0.5, -0.8, 0.2)));
sh::Factory::getInstance ().setSharedParameter ("waterSunFade_sunHeight", sh::makeProperty<sh::Vector2>(new sh::Vector2(1, 0.6)));
sh::Factory::getInstance ().setSharedParameter ("vpRow2Fix", sh::makeProperty<sh::Vector4> (new sh::Vector4(0,0,0,0)));
#endif
    }

    page = "Scene";
    {
        Setting *fastFactor = createSetting (Type_SpinBox, page, "fast factor");
        fastFactor->setDefaultValue(4);
        fastFactor->setEditorSetting(false);
        fastFactor->setColumnSpan (1);
        // FIXME: setMinimum or setSpecialVlueText appears to be broken, possibly due
        // to there being an empty string default for special value text.
        fastFactor->setMinimum (1);
        fastFactor->setSpecialValueText("1"); // workaround for above
        fastFactor->setMaximum (100); // FIXME: not sure what the max value should be
        fastFactor->setWidgetWidth (10);
        fastFactor->setViewLocation(1, 2);
        Setting *ffText = createSetting (Type_Undefined, page, "ffText");
        ffText->setSpecialValueText("Fast Factor"); // hack to place text labels
        ffText->setEditorSetting(false);
        ffText->setSerializable (false);
        ffText->setColumnSpan (1);
        ffText->setWidgetWidth (10);
        ffText->setViewLocation(1, 1);

        Setting *farClipDist = createSetting (Type_SpinBox, page, "far clip distance");
        farClipDist->setDefaultValue(300000);
        farClipDist->setEditorSetting(false);
        farClipDist->setColumnSpan (1);
        farClipDist->setMinimum (0);
        farClipDist->setMaximum (1000000); // FIXME: not sure what the max value should be
        farClipDist->setWidgetWidth (10);
        farClipDist->setViewLocation(2, 2);
        Setting *fcText = createSetting (Type_Undefined, page, "fcText");
        fcText->setSpecialValueText("Far Clip Distance");
        fcText->setEditorSetting(false);
        fcText->setSerializable (false);
        fcText->setColumnSpan (1);
        fcText->setWidgetWidth (10);
        fcText->setViewLocation(2, 1);

        Setting *timerStart = createSetting (Type_SpinBox, page, "timer start");
        timerStart->setDefaultValue(20);
        timerStart->setEditorSetting(false);
        timerStart->setColumnSpan (1);
        timerStart->setMinimum (0);
        timerStart->setMaximum (100); // FIXME: not sure what the max value should be
        timerStart->setWidgetWidth (10);
        timerStart->setViewLocation(3, 2);
        Setting *tsText = createSetting (Type_Undefined, page, "tsText");
        tsText->setSpecialValueText("Timer Start");
        tsText->setEditorSetting(false);
        tsText->setSerializable (false);
        tsText->setColumnSpan (1);
        tsText->setWidgetWidth (10);
        tsText->setViewLocation(3, 1);
    }

#if 0
    page = "Window Size";
    {
        Setting *width = createSetting (Type_LineEdit, page, "Width");
        Setting *height = createSetting (Type_LineEdit, page, "Height");

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
        Setting *preDefined = createSetting (Type_ComboBox, page,
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

    page = "Display Format";
    {
        QString defaultValue = "Icon and Text";

        QStringList values = QStringList()
                            << defaultValue << "Icon Only" << "Text Only";

        Setting *rsd = createSetting (Type_RadioButton,
                                      page, "Record Status Display");

        Setting *ritd = createSetting (Type_RadioButton,
                                      page, "Referenceable ID Type Display");

        rsd->setDeclaredValues (values);
        ritd->setDeclaredValues (values);

        rsd->setEditorSetting (true);
        ritd->setEditorSetting (true);
    }
#endif

    page = "Proxy Selection Test";
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

        Setting *masterBoolean = createSetting (Type_RadioButton, page,
                                                "Master Proxy");

        Setting *slaveBoolean = createSetting (Type_CheckBox, page,
                                                "Proxy Checkboxes");

        Setting *slaveSingleText = createSetting (Type_LineEdit, page,
                                                "Proxy TextBox 1");

        Setting *slaveMultiText = createSetting (Type_LineEdit, page,
                                                "ProxyTextBox 2");

        Setting *slaveAlphaSpinbox = createSetting (Type_SpinBox, page,
                                                "Alpha Spinbox");

        Setting *slaveIntegerSpinbox = createSetting (Type_SpinBox, page,
                                                "Int Spinbox");

        Setting *slaveDoubleSpinbox = createSetting (Type_DoubleSpinBox,
                                                page, "Double Spinbox");

        Setting *slaveSlider = createSetting (Type_Slider, page, "Slider");

        Setting *slaveDial = createSetting (Type_Dial, page, "Dial");

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

UserSettings::~UserSettings()
{
    mUserSettingsInstance = 0;
}

void UserSettings::loadSettings (const QString &fileName)
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

    // prepare to use the settings from settings.cfg
    const std::string localdefault = mCfgMgr.getLocalPath().string() + "/settings-default.cfg";
    const std::string globaldefault = mCfgMgr.getGlobalPath().string() + "/settings-default.cfg";

    Settings::Manager settings;
    // prefer local
    if (boost::filesystem::exists(localdefault))
        settings.loadDefault(localdefault);
    else if (boost::filesystem::exists(globaldefault))
        settings.loadDefault(globaldefault);
    else
        std::cerr<< "No default settings file found! Make sure the file \"settings-default.cfg\" was properly installed."<< std::endl;

    // load user settings if they exist, otherwise just load the default settings as user settings
    const std::string settingspath = mCfgMgr.getUserConfigPath().string() + "/settings.cfg";
    if (boost::filesystem::exists(settingspath))
        settings.loadUser(settingspath);
    else if (boost::filesystem::exists(localdefault))
        settings.loadUser(localdefault);
    else if (boost::filesystem::exists(globaldefault))
        settings.loadUser(globaldefault);

    std::string renderSystem = settings.getString("render system", "Video");
    if(renderSystem == "")
    {
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
        renderSystem = "Direct3D9 Rendering Subsystem";
#else
        renderSystem = "OpenGL Rendering Subsystem";
#endif
    }
    mSettingCfgDefinitions->setValue("Video/render system", renderSystem.c_str());

    std::string currShader = settings.getString("shader mode", "General");
    // can't call Ogre::Root at this point as it hasn't been initialised
    QString rend = renderSystem.c_str();
    bool openGL = rend.contains(QRegExp("^OpenGL", Qt::CaseInsensitive));
    bool glES = rend.contains(QRegExp("^OpenGL ES", Qt::CaseInsensitive));

    // force shader language based on render system
    if(currShader == ""
            || (openGL && currShader == "hlsl")
            || (!openGL && currShader == "glsl")
            || (glES && currShader != "glsles"))
    {
        QString shader = openGL ? (glES ? "glsles" : "glsl") : "hlsl";
        mSettingDefinitions->setValue("shader mode", shader); //no group means "General" group
    }

    // check if override entry exists (default: override)
    if(!mSettingDefinitions->childGroups().contains("Video", Qt::CaseInsensitive))
        mSettingDefinitions->setValue("Video/use settings.cfg", "true");
}

QStringList UserSettings::getOgreRenderers()
{
    if(mOgreRenderers.empty())
    {
        Ogre::RenderSystemList renderers = Ogre::Root::getSingleton().getAvailableRenderers();
        Ogre::RenderSystemList::iterator it = renderers.begin();
        for(; it != renderers.end(); ++it)
            mOgreRenderers.append((*it)->getName().c_str());
    }

    return mOgreRenderers;
}

QStringList UserSettings::getOgreOptions(const QString &key, const QString &renderer)
{
    QStringList result;

    Ogre::RenderSystem *rend = Ogre::Root::getSingleton().getRenderSystemByName(renderer.toStdString());
    if(!rend)
        return result;

    Ogre::ConfigOptionMap& renderOpt = rend->getConfigOptions();
    Ogre::ConfigOptionMap::iterator it = renderOpt.begin();

    uint row = 0;
    for(; it != renderOpt.end(); ++it, ++row)
    {
        Ogre::StringVector::iterator opt_it = it->second.possibleValues.begin();
        uint idx = 0;

        for(; opt_it != it->second.possibleValues.end(); ++opt_it, ++idx)
        {
            if(strcmp (key.toStdString().c_str(), it->first.c_str()) == 0)
            {
                result << ((key == "FSAA") ? QString("MSAA ") : QString(""))
                    + QString::fromStdString((*opt_it).c_str()).simplified();
            }
        }
    }

    // Sort ascending
    qSort(result.begin(), result.end(), naturalSortLessThanCI);

    // Replace the zero option with Off
    int index = result.indexOf("MSAA 0");

    if(index != -1)
        result.replace(index, QObject::tr("Off"));

    return result;
}

QStringList UserSettings::getShaderLanguageByRenderer(const QString &renderer)
{
    QStringList result;

    if(renderer == "Direct3D9 Rendering Subsystem")
        result.append("HLSL");
    else if(renderer == "OpenGL Rendering Subsystem")
        result.append("GLSL");
    else if(renderer.contains(QRegExp("^OpenGL ES", Qt::CaseInsensitive)))
        result.append("GLSLES");

    return result;
}

bool UserSettings::hasSettingDefinitions (const QString &viewKey) const
{
    return (mSettingDefinitions->contains (viewKey));
}

void UserSettings::setDefinitions (const QString &key, const QStringList &list)
{
    mSettingDefinitions->setValue (key, list);
}

void UserSettings::saveDefinitions() const
{
    mSettingDefinitions->sync();
}

QString UserSettings::settingValue (const QString &settingKey)
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

UserSettings& UserSettings::instance()
{
    assert(mUserSettingsInstance);
    return *mUserSettingsInstance;
}

void UserSettings::updateUserSetting(const QString &settingKey, const QStringList &list)
{
    mSettingDefinitions->setValue (settingKey ,list);

    emit userSettingUpdated (settingKey, list);
}

Setting *UserSettings::findSetting (const QString &pageName, const QString &settingName)
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

void UserSettings::removeSetting (const QString &pageName, const QString &settingName)
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

SettingPageMap UserSettings::settingPageMap() const
{
    SettingPageMap pageMap;

    foreach (Setting *setting, mSettings)
        pageMap[setting->page()].append (setting);

    return pageMap;
}

Setting *UserSettings::createSetting
        (SettingType typ, const QString &page, const QString &name)
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

QStringList UserSettings::definitions (const QString &viewKey) const
{
    if (mSettingDefinitions->contains (viewKey))
        return mSettingDefinitions->value (viewKey).toStringList();

    return QStringList();
}

}
