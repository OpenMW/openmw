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

CSMSettings::UserSettings *CSMSettings::UserSettings::sUserSettingsInstance = 0;

    CSMSettings::UserSettings::UserSettings (const Files::ConfigurationManager& configurationManager)
    : mCfgMgr (configurationManager)
    , mSettingDefinitions(NULL)
{
    assert(!sUserSettingsInstance);
    sUserSettingsInstance = this;

    buildSettingModelDefaults();
}

void CSMSettings::UserSettings::buildSettingModelDefaults()
{
    QString section;

    declareSection ("3d-render", "3D Rendering");
    {
        Setting *shaders = createSetting (Type_CheckBox, "shaders", "Enable Shaders");
        shaders->setDefaultValue ("true");

        Setting *farClipDist = createSetting (Type_DoubleSpinBox, "far-clip-distance", "Far clipping distance");
        farClipDist->setDefaultValue (300000);
        farClipDist->setRange (0, 1000000);
        farClipDist->setToolTip ("The maximum distance objects are still rendered at.");

        QString defaultValue = "None";
        Setting *antialiasing = createSetting (Type_ComboBox, "antialiasing", "Antialiasing");
        antialiasing->setDeclaredValues (QStringList()
            << defaultValue << "MSAA 2" << "MSAA 4" << "MSAA 8" << "MSAA 16");
        antialiasing->setDefaultValue (defaultValue);
    }

    declareSection ("3d-render-adv", "3D Rendering (Advanced)");
    {
        Setting *numLights = createSetting (Type_SpinBox, "num_lights",
            "Number of lights per pass");
        numLights->setDefaultValue (8);
        numLights->setRange (1, 100);
    }

    declareSection ("scene-input", "Scene Input");
    {
        Setting *timer = createSetting (Type_SpinBox, "timer", "Input responsiveness");
        timer->setDefaultValue (20);
        timer->setRange (1, 100);
        timer->setToolTip ("The time between two checks for user input in milliseconds.<p>"
            "Lower value result in higher responsiveness.");

        Setting *fastFactor = createSetting (Type_SpinBox, "fast-factor",
            "Fast movement factor");
        fastFactor->setDefaultValue (4);
        fastFactor->setRange (1, 100);
        fastFactor->setToolTip (
            "Factor by which movement is speed up while the shift key is held down.");
    }

    declareSection ("window", "Window");
    {
        Setting *preDefined = createSetting (Type_ComboBox, "pre-defined",
            "Default window size");
        preDefined->setEditorSetting (false);
        preDefined->setDeclaredValues (
            QStringList() << "640 x 480" << "800 x 600" << "1024 x 768" << "1440 x 900");
        preDefined->setViewLocation (1, 1);
        preDefined->setColumnSpan (2);
        preDefined->setToolTip ("Newly opened top-level windows will open with this size "
            "(picked from a list of pre-defined values)");

        Setting *width = createSetting (Type_LineEdit, "default-width",
            "Default window width");
        width->setDefaultValues (QStringList() << "1024");
        width->setViewLocation (2, 1);
        width->setColumnSpan (1);
        width->setToolTip ("Newly opened top-level windows will open with this width.");
        preDefined->addProxy (width, QStringList() << "640" << "800" << "1024" << "1440");

        Setting *height = createSetting (Type_LineEdit, "default-height",
            "Default window height");
        height->setDefaultValues (QStringList() << "768");
        height->setViewLocation (2, 2);
        height->setColumnSpan (1);
        height->setToolTip ("Newly opened top-level windows will open with this height.");
        preDefined->addProxy (height, QStringList() << "480" << "600" << "768" << "900");

        Setting *reuse = createSetting (Type_CheckBox, "reuse", "Reuse Subviews");
        reuse->setDefaultValue ("true");
        reuse->setToolTip ("When a new subview is requested and a matching subview already "
            " exist, do not open a new subview and use the existing one instead.");

        Setting *statusBar = createSetting (Type_CheckBox, "show-statusbar", "Show Status Bar");
        statusBar->setDefaultValue ("true");
        statusBar->setToolTip ("If a newly open top level window is showing status bars or not. "
            " Note that this does not affect existing windows.");

        Setting *maxSubView = createSetting (Type_SpinBox, "max-subviews",
            "Maximum number of subviews per top-level window");
        maxSubView->setDefaultValue (256);
        maxSubView->setRange (1, 256);
        maxSubView->setToolTip ("If the maximum number is reached and a new subview is opened "
            "it will be placed into a new top-level window.");

        Setting *hide = createSetting (Type_CheckBox, "hide-subview", "Hide single subview");
        hide->setDefaultValue ("false");
        hide->setToolTip ("When a view contains only a single subview, hide the subview title "
            "bar and if this subview is closed also close the view (unless it is the last "
            "view for this document)");

        Setting *minWidth = createSetting (Type_SpinBox, "minimum-width",
            "Minimum subview width");
        minWidth->setDefaultValue (325);
        minWidth->setRange (50, 10000);
        minWidth->setToolTip ("Minimum width of subviews.");
    }

    declareSection ("records", "Records");
    {
        QString defaultValue = "Icon and Text";
        QStringList values = QStringList() << defaultValue << "Icon Only" << "Text Only";

        Setting *rsd = createSetting (Type_RadioButton, "status-format",
            "Modification status display format");
        rsd->setDefaultValue (defaultValue);
        rsd->setDeclaredValues (values);

        Setting *ritd = createSetting (Type_RadioButton, "type-format",
            "ID type display format");
        ritd->setDefaultValue (defaultValue);
        ritd->setDeclaredValues (values);
    }

    declareSection ("table-input", "Table Input");
    {
        QString inPlaceEdit ("Edit in Place");
        QString editRecord ("Edit Record");
        QString view ("View");
        QString editRecordAndClose ("Edit Record and Close");

        QStringList values;
        values
            << "None" << inPlaceEdit << editRecord << view << "Revert" << "Delete"
            << editRecordAndClose << "View and Close";

        QString toolTip = "<ul>"
            "<li>None</li>"
            "<li>Edit in Place: Edit the clicked cell</li>"
            "<li>Edit Record: Open a dialogue subview for the clicked record</li>"
            "<li>View: Open a scene subview for the clicked record (not available everywhere)</li>"
            "<li>Revert: Revert record</li>"
            "<li>Delete: Delete recordy</li>"
            "<li>Edit Record and Close: Open a dialogue subview for the clicked record and close the table subview</li>"
            "<li>View And Close: Open a scene subview for the clicked record and close the table subview</li>"
            "</ul>";

        Setting *doubleClick = createSetting (Type_ComboBox, "double", "Double Click");
        doubleClick->setDeclaredValues (values);
        doubleClick->setDefaultValue (inPlaceEdit);
        doubleClick->setToolTip ("Action on double click in table:<p>" + toolTip);

        Setting *shiftDoubleClick = createSetting (Type_ComboBox, "double-s",
            "Shift Double Click");
        shiftDoubleClick->setDeclaredValues (values);
        shiftDoubleClick->setDefaultValue (editRecord);
        shiftDoubleClick->setToolTip ("Action on shift double click in table:<p>" + toolTip);

        Setting *ctrlDoubleClick = createSetting (Type_ComboBox, "double-c",
            "Control Double Click");
        ctrlDoubleClick->setDeclaredValues (values);
        ctrlDoubleClick->setDefaultValue (view);
        ctrlDoubleClick->setToolTip ("Action on control double click in table:<p>" + toolTip);

        Setting *shiftCtrlDoubleClick = createSetting (Type_ComboBox, "double-sc",
            "Shift Control Double Click");
        shiftCtrlDoubleClick->setDeclaredValues (values);
        shiftCtrlDoubleClick->setDefaultValue (editRecordAndClose);
        shiftCtrlDoubleClick->setToolTip ("Action on shift control double click in table:<p>" + toolTip);
    }

    declareSection ("search", "Search & Replace");
    {
        Setting *before = createSetting (Type_SpinBox, "char-before",
            "Characters before search string");
        before->setDefaultValue (10);
        before->setRange (0, 1000);
        before->setToolTip ("Maximum number of character to display in search result before the searched text");

        Setting *after = createSetting (Type_SpinBox, "char-after",
            "Characters after search string");
        after->setDefaultValue (10);
        after->setRange (0, 1000);
        after->setToolTip ("Maximum number of character to display in search result after the searched text");

        Setting *autoDelete = createSetting (Type_CheckBox, "auto-delete", "Delete row from result table after a successful replace");
        autoDelete->setDefaultValue ("true");
    }

    declareSection ("script-editor", "Script Editor");
    {
        Setting *lineNum = createSetting (Type_CheckBox, "show-linenum", "Show Line Numbers");
        lineNum->setDefaultValue ("true");
        lineNum->setToolTip ("Show line numbers to the left of the script editor window."
                "The current row and column numbers of the text cursor are shown at the bottom.");

        Setting *monoFont = createSetting (Type_CheckBox, "mono-font", "Use monospace font");
        monoFont->setDefaultValue ("true");
        monoFont->setToolTip ("Whether to use monospaced fonts on script edit subview.");
    }

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
    sUserSettingsInstance = 0;
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
}

// if the key is not found create one with a default value
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

    if (!mSettingDefinitions->contains (settingKey))
        return QString();

    defs = mSettingDefinitions->value (settingKey).toStringList();

    if (defs.isEmpty())
        return QString();

    return defs.at(0);
}

CSMSettings::UserSettings& CSMSettings::UserSettings::instance()
{
    assert(sUserSettingsInstance);
    return *sUserSettingsInstance;
}

void CSMSettings::UserSettings::updateUserSetting(const QString &settingKey,
                                                    const QStringList &list)
{
    mSettingDefinitions->setValue (settingKey ,list);

    if(settingKey == "3d-render-adv/num_lights" && !list.empty())
    {
        sh::Factory::getInstance ().setGlobalSetting ("num_lights", list.at(0).toStdString());
    }
    else if(settingKey == "3d-render/shaders" && !list.empty())
    {
        sh::Factory::getInstance ().setShadersEnabled (list.at(0).toStdString() == "true" ? true : false);
    }

    emit userSettingUpdated (settingKey, list);
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
    {
        SettingPageMap::iterator iter = pageMap.find (setting->page());

        if (iter==pageMap.end())
        {
            QPair<QString, QList <Setting *> > value;

            std::map<QString, QString>::const_iterator iter2 =
                mSectionLabels.find (setting->page());

            value.first = iter2!=mSectionLabels.end() ? iter2->second : "";

            iter = pageMap.insert (setting->page(), value);
        }

        iter->second.append (setting);
    }

    return pageMap;
}

CSMSettings::Setting *CSMSettings::UserSettings::createSetting
        (CSMSettings::SettingType type, const QString &name, const QString& label)
{
    Setting *setting = new Setting (type, name, mSection, label);

    // set useful defaults
    int row = 1;

    if (!mSettings.empty())
        row = mSettings.back()->viewRow()+1;

    setting->setViewLocation (row, 1);

    setting->setColumnSpan (3);

    int width = 10;

    if (type==Type_CheckBox)
        width = 40;

    setting->setWidgetWidth (width);

    if (type==Type_CheckBox)
        setting->setStyleSheet ("QGroupBox { border: 0px; }");

    if (type==Type_CheckBox)
        setting->setDeclaredValues(QStringList() << "true" << "false");

    if (type==Type_CheckBox)
        setting->setSpecialValueText (setting->getLabel());

    //add declaration to the model
    mSettings.append (setting);

    return setting;
}

void CSMSettings::UserSettings::declareSection (const QString& page, const QString& label)
{
    mSection = page;
    mSectionLabels[page] = label;
}

QStringList CSMSettings::UserSettings::definitions (const QString &viewKey) const
{
    if (mSettingDefinitions->contains (viewKey))
        return mSettingDefinitions->value (viewKey).toStringList();

    return QStringList();
}
