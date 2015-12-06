
#include "state.hpp"

#include <stdexcept>
#include <algorithm>

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
    declareCategory ("window", "Windows");

    declareCategory ("records", "Records");

    declareCategory ("table-input", "ID Tables");

    declareCategory ("dialogues", "ID Dialogues");

    declareCategory ("report-input", "Reports");

    declareCategory ("search", "Search & Replace");

    declareCategory ("script-editor", "Scripts");

    declareCategory ("general-input", "General Input");

    declareCategory ("scene-input", "3D Scene Input");

    declareCategory ("tooltips", "Tooltips");
}

void CSMPrefs::State::declareCategory (const std::string& key, const std::string& name)
{
    std::map<std::string, Category>::iterator iter = mCategories.find (key);

    if (iter!=mCategories.end())
    {
        mCurrentCategory = iter;
    }
    else
    {
        mCurrentCategory =
            mCategories.insert (std::make_pair (key, Category (this, key, name))).first;
    }
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

std::vector<std::pair<std::string, std::string> > CSMPrefs::State::listCategories() const
{
    std::vector<std::pair<std::string, std::string> > list;

    for (std::map<std::string, Category>::const_iterator iter (mCategories.begin());
        iter!=mCategories.end(); ++iter)
        list.push_back (std::make_pair (iter->second.getName(), iter->first));

    std::sort (list.begin(), list.end());

    return list;
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
