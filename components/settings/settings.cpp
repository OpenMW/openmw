#include "settings.hpp"
#include "parser.hpp"

#include <components/debug/debuglog.hpp>
#include <components/misc/stringops.hpp>

#include <boost/lexical_cast.hpp>

namespace Settings
{

CategorySettingValueMap Manager::mDefaultSettings = CategorySettingValueMap();
CategorySettingValueMap Manager::mUserSettings = CategorySettingValueMap();
CategorySettingVector Manager::mChangedSettings = CategorySettingVector();

void Manager::clear()
{
    mDefaultSettings.clear();
    mUserSettings.clear();
    mChangedSettings.clear();
}

void Manager::loadDefault(const std::string &file)
{
    SettingsFileParser parser;
    parser.loadSettingsFile(file, mDefaultSettings);
}

void Manager::loadUser(const std::string &file)
{
    SettingsFileParser parser;
    parser.loadSettingsFile(file, mUserSettings);
}

void Manager::saveUser(const std::string &file)
{
    SettingsFileParser parser;
    parser.saveSettingsFile(file, mUserSettings);
}

std::string Manager::getString(const std::string &setting, const std::string &category)
{
    CategorySettingValueMap::key_type key = std::make_pair(category, setting);
    CategorySettingValueMap::iterator it = mUserSettings.find(key);
    if (it != mUserSettings.end())
        return it->second;

    it = mDefaultSettings.find(key);
    if (it != mDefaultSettings.end())
        return it->second;

    throw std::runtime_error(std::string("Trying to retrieve a non-existing setting: ") + setting
                             + ".\nMake sure the settings-default.cfg file was properly installed.");
}

float Manager::getFloat (const std::string& setting, const std::string& category)
{
    const std::string value = getString(setting, category);
    try
    {
        // We have to rely on Boost because std::stof from C++11 uses the current locale
        // for separators (which is undesired) and it often silently ignores parsing errors.
        return boost::lexical_cast<float>(value);
    }
    catch (boost::bad_lexical_cast&)
    {
        Log(Debug::Warning) << "Cannot parse setting '" << setting << "' (invalid setting value: " << value << ").";
        return 0.f;
    }
}

int Manager::getInt (const std::string& setting, const std::string& category)
{
    const std::string value = getString(setting, category);
    try
    {
        return std::stoi(value);
    }
    catch(const std::exception& e)
    {
        Log(Debug::Warning) << "Cannot parse setting '" << setting << "' (invalid setting value: " << value << ").";
        return 0;
    }
}

bool Manager::getBool (const std::string& setting, const std::string& category)
{
    const std::string& string = getString(setting, category);
    return Misc::StringUtils::ciEqual(string, "true");
}

void Manager::setString(const std::string &setting, const std::string &category, const std::string &value)
{
    CategorySettingValueMap::key_type key = std::make_pair(category, setting);

    CategorySettingValueMap::iterator found = mUserSettings.find(key);
    if (found != mUserSettings.end())
    {
        if (found->second == value)
            return;
    }

    mUserSettings[key] = value;

    mChangedSettings.insert(key);
}

void Manager::setInt (const std::string& setting, const std::string& category, const int value)
{
    std::ostringstream stream;
    stream << value;
    setString(setting, category, stream.str());
}

void Manager::setFloat (const std::string &setting, const std::string &category, const float value)
{
    std::ostringstream stream;
    stream << value;
    setString(setting, category, stream.str());
}

void Manager::setBool(const std::string &setting, const std::string &category, const bool value)
{
    setString(setting, category, value ? "true" : "false");
}

void Manager::resetPendingChange(const std::string &setting, const std::string &category)
{
    CategorySettingValueMap::key_type key = std::make_pair(category, setting);
    mChangedSettings.erase(key);
}

const CategorySettingVector Manager::getPendingChanges()
{
    return mChangedSettings;
}

void Manager::resetPendingChanges()
{
    mChangedSettings.clear();
}

}
