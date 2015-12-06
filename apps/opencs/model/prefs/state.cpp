
#include "state.hpp"

#include <stdexcept>

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

}

CSMPrefs::State::State (const Files::ConfigurationManager& configurationManager)
: mConfigFile ("opencs.ini"), mConfigurationManager (configurationManager)
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
