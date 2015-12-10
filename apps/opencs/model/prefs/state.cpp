
#include "state.hpp"

#include <stdexcept>
#include <algorithm>
#include <sstream>

#include "intsetting.hpp"
#include "doublesetting.hpp"

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
    // reuse
    // show-statusbar
    declareInt ("max-subviews", "Maximum number of subviews per top-level window", 256).
        setTooltip ("If the maximum number is reached and a new subview is opened "
            "it will be placed into a new top-level window.").
        setRange (1, 256);
    // hide-subview
    declareInt ("minimum-width", "Minimum subview width", 325).
        setTooltip ("Minimum width of subviews.").
        setRange (50, 10000);
    // mainwindow-scrollbar
    // grow-limit

    declareCategory ("Records");

    declareCategory ("ID Tables");

    declareCategory ("ID Dialogues");

    declareCategory ("Reports");

    declareCategory ("Search & Replace");
    declareInt ("char-before", "Characters before search string", 10).
        setTooltip ("Maximum number of character to display in search result before the searched text");
    declareInt ("char-after", "Characters after search string", 10).
        setTooltip ("Maximum number of character to display in search result after the searched text");
    // auto-delete

    declareCategory ("Scripts");
    // show-linenum
    // mono-font
    // warnings
    // toolbar
    declareInt ("compile-delay", "Delay between updating of source errors", 100).
        setTooltip ("Delay in milliseconds").
        setRange (0, 10000);
    declareInt ("error-height", "Initial height of the error panel", 100).
        setRange (100, 10000);
    // syntax-colouring

    declareCategory ("General Input");

    declareCategory ("3D Scene Input");
    // p-navi
    // s-navi
    // p-edit
    // s-edit
    // p-select
    // s-select
    // context-select
    declareDouble ("drag-factor", "Mouse sensitivity during drag operations", 1.0).
        setRange (0.001, 100.0);
    declareDouble ("drag-wheel-factor", "Mouse wheel sensitivity during drag operations", 1.0).
        setRange (0.001, 100.0);
    declareDouble ("drag-shift-factor",
            "Shift-acceleration factor during drag operations", 4.0).
        setTooltip ("Acceleration factor during drag operations while holding down shift").
        setRange (0.001, 100.0);

    declareCategory ("Tooltips");
    // scene
    // scene-hide-basic
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
