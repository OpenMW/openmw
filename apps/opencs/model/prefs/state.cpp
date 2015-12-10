
#include "state.hpp"

#include <stdexcept>
#include <algorithm>
#include <sstream>

#include "intsetting.hpp"
#include "doublesetting.hpp"
#include "boolsetting.hpp"

CSMPrefs::State *CSMPrefs::State::sThis = 0;

void CSMPrefs::State::load()
{
    // default settings file
    boost::filesystem::path local = mConfigurationManager.getLocalPath() / mConfigFile;
    boost::filesystem::path global = mConfigurationManager.getGlobalPath() / mConfigFile;

    if (boost::filesystem::exists (local))
        mSettings.loadDefault (local.string());
    else if (boost::filesystem::exists (global))
        mSettings.loadDefault (global.string());
    else
        throw std::runtime_error ("No default settings file found! Make sure the file \"opencs.ini\" was properly installed.");

    // user settings file
    boost::filesystem::path user = mConfigurationManager.getUserConfigPath() / mConfigFile;

    if (boost::filesystem::exists (user))
        mSettings.loadUser (user.string());
}

void CSMPrefs::State::declare()
{
    declareCategory ("Windows");
    declareInt ("default-width", "Default window width", 800).
        setTooltip ("Newly opened top-level windows will open with this width.").
        setMin (80);
    declareInt ("default-height", "Default window height", 600).
        setTooltip ("Newly opened top-level windows will open with this height.").
        setMin (80);
    declareBool ("reuse", "Reuse Subviews", true).
        setTooltip ("When a new subview is requested and a matching subview already "
        " exist, do not open a new subview and use the existing one instead.");
    declareBool ("show-statusbar", "Show Status Bar", true).
        setTooltip ("If a newly open top level window is showing status bars or not. "
        " Note that this does not affect existing windows.");
    declareInt ("max-subviews", "Maximum number of subviews per top-level window", 256).
        setTooltip ("If the maximum number is reached and a new subview is opened "
            "it will be placed into a new top-level window.").
        setRange (1, 256);
    declareBool ("hide-subview", "Hide single subview", false).
        setTooltip ("When a view contains only a single subview, hide the subview title "
        "bar and if this subview is closed also close the view (unless it is the last "
        "view for this document)");
    declareInt ("minimum-width", "Minimum subview width", 325).
        setTooltip ("Minimum width of subviews.").
        setRange (50, 10000);
    // mainwindow-scrollbar
    declareBool ("grow-limit", "Grow Limit Screen", false).
        setTooltip ("When \"Grow then Scroll\" option is selected, the window size grows to"
        " the width of the virtual desktop. \nIf this option is selected the the window growth"
        "is limited to the current screen.");

    declareCategory ("Records");

    declareCategory ("ID Tables");
    // double
    // double-s
    // double-c
    // double-sc
    // jump-to-added
    declareBool ("extended-config",
        "Manually specify affected record types for an extended delete/revert", false).
        setTooltip ("Delete and revert commands have an extended form that also affects "
        "associated records.\n\n"
        "If this option is enabled, types of affected records are selected "
        "manually before a command execution.\nOtherwise, all associated "
        "records are deleted/reverted immediately.");

    declareCategory ("ID Dialogues");
    declareBool ("toolbar", "Show toolbar", true);

    declareCategory ("Reports");

    declareCategory ("Search & Replace");
    declareInt ("char-before", "Characters before search string", 10).
        setTooltip ("Maximum number of character to display in search result before the searched text");
    declareInt ("char-after", "Characters after search string", 10).
        setTooltip ("Maximum number of character to display in search result after the searched text");
    declareBool ("auto-delete", "Delete row from result table after a successful replace", true);

    declareCategory ("Scripts");
    declareBool ("show-linenum", "Show Line Numbers", true).
        setTooltip ("Show line numbers to the left of the script editor window."
        "The current row and column numbers of the text cursor are shown at the bottom.");
    declareBool ("mono-font", "Use monospace font", true);
    // warnings
    declareBool ("toolbar", "Show toolbar", true);
    declareInt ("compile-delay", "Delay between updating of source errors", 100).
        setTooltip ("Delay in milliseconds").
        setRange (0, 10000);
    declareInt ("error-height", "Initial height of the error panel", 100).
        setRange (100, 10000);
    // syntax-colouring

    declareCategory ("General Input");
    declareBool ("cycle", "Cyclic next/previous", false).
        setTooltip ("When using next/previous functions at the last/first item of a "
        "list go to the first/last item");

    declareCategory ("3D Scene Input");
    // p-navi
    // s-navi
    // p-edit
    // s-edit
    // p-select
    // s-select
    declareBool ("context-select", "Context Sensitive Selection", false);
    declareDouble ("drag-factor", "Mouse sensitivity during drag operations", 1.0).
        setRange (0.001, 100.0);
    declareDouble ("drag-wheel-factor", "Mouse wheel sensitivity during drag operations", 1.0).
        setRange (0.001, 100.0);
    declareDouble ("drag-shift-factor",
            "Shift-acceleration factor during drag operations", 4.0).
        setTooltip ("Acceleration factor during drag operations while holding down shift").
        setRange (0.001, 100.0);

    declareCategory ("Tooltips");
    declareBool ("scene", "Show Tooltips in 3D scenes", true);
    declareBool ("scene-hide-basic", "Hide basic  3D scenes tooltips", false);
    declareInt ("scene-delay", "Tooltip delay in milliseconds", 500).
        setMin (1);
}

void CSMPrefs::State::declareCategory (const std::string& key)
{
    std::map<std::string, Category>::iterator iter = mCategories.find (key);

    if (iter!=mCategories.end())
    {
        mCurrentCategory = iter;
    }
    else
    {
        mCurrentCategory =
            mCategories.insert (std::make_pair (key, Category (this, key))).first;
    }
}

CSMPrefs::IntSetting& CSMPrefs::State::declareInt (const std::string& key,
    const std::string& label, int default_)
{
    if (mCurrentCategory==mCategories.end())
        throw std::logic_error ("no category for setting");

    std::ostringstream stream;
    stream << default_;
    setDefault (key, stream.str());

    default_ = mSettings.getInt (key, mCurrentCategory->second.getKey());

    CSMPrefs::IntSetting *setting =
        new CSMPrefs::IntSetting (&mCurrentCategory->second, &mSettings, key, label, default_);

    mCurrentCategory->second.addSetting (setting);

    return *setting;
}

CSMPrefs::DoubleSetting& CSMPrefs::State::declareDouble (const std::string& key,
    const std::string& label, double default_)
{
    if (mCurrentCategory==mCategories.end())
        throw std::logic_error ("no category for setting");

    std::ostringstream stream;
    stream << default_;
    setDefault (key, stream.str());

    default_ = mSettings.getFloat (key, mCurrentCategory->second.getKey());

    CSMPrefs::DoubleSetting *setting =
        new CSMPrefs::DoubleSetting (&mCurrentCategory->second, &mSettings, key, label, default_);

    mCurrentCategory->second.addSetting (setting);

    return *setting;
}

CSMPrefs::BoolSetting& CSMPrefs::State::declareBool (const std::string& key,
    const std::string& label, bool default_)
{
    if (mCurrentCategory==mCategories.end())
        throw std::logic_error ("no category for setting");

    setDefault (key, default_ ? "true" : "false");

    default_ = mSettings.getBool (key, mCurrentCategory->second.getKey());

    CSMPrefs::BoolSetting *setting =
        new CSMPrefs::BoolSetting (&mCurrentCategory->second, &mSettings, key, label, default_);

    mCurrentCategory->second.addSetting (setting);

    return *setting;
}

void CSMPrefs::State::setDefault (const std::string& key, const std::string& default_)
{
    Settings::CategorySetting fullKey (mCurrentCategory->second.getKey(), key);

    Settings::CategorySettingValueMap::iterator iter =
        mSettings.mDefaultSettings.find (fullKey);

    if (iter==mSettings.mDefaultSettings.end())
        mSettings.mDefaultSettings.insert (std::make_pair (fullKey, default_));
}

CSMPrefs::State::State (const Files::ConfigurationManager& configurationManager)
: mConfigFile ("opencs.ini"), mConfigurationManager (configurationManager),
  mCurrentCategory (mCategories.end())
{
    if (sThis)
        throw std::logic_error ("An instance of CSMPRefs::State already exists");

    load();
    declare();

    sThis = this;
}

CSMPrefs::State::~State()
{
    sThis = 0;
}

void CSMPrefs::State::save()
{
    boost::filesystem::path user = mConfigurationManager.getUserConfigPath() / mConfigFile;
    mSettings.saveUser (user.string());
}

CSMPrefs::State::Iterator CSMPrefs::State::begin()
{
    return mCategories.begin();
}

CSMPrefs::State::Iterator CSMPrefs::State::end()
{
    return mCategories.end();
}

CSMPrefs::Category& CSMPrefs::State::getCategory (const std::string& key)
{
    Iterator iter = mCategories.find (key);

    if (iter==mCategories.end())
        throw std::logic_error ("Invalid user settings category: " + key);

    return iter->second;
}

void CSMPrefs::State::update (const Setting& setting)
{
    emit (settingChanged (setting));
}

CSMPrefs::State& CSMPrefs::State::get()
{
    if (!sThis)
        throw std::logic_error ("No instance of CSMPrefs::State");

    return *sThis;
}


CSMPrefs::State& CSMPrefs::get()
{
    return State::get();
}
