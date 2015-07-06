#include "settings.hpp"

#include <stdexcept>
#include <sstream>

#include <components/misc/stringops.hpp>

#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/algorithm/string.hpp>

namespace
{

    bool parseBool(const std::string& string)
    {
        return (Misc::StringUtils::ciEqual(string, "true"));
    }

    float parseFloat(const std::string& string)
    {
        std::stringstream stream;
        stream << string;
        float ret = 0.f;
        stream >> ret;
        return ret;
    }

    int parseInt(const std::string& string)
    {
        std::stringstream stream;
        stream << string;
        int ret = 0;
        stream >> ret;
        return ret;
    }

    template <typename T>
    std::string toString(T val)
    {
        std::ostringstream stream;
        stream << val;
        return stream.str();
    }

    template <>
    std::string toString(bool val)
    {
        return val ? "true" : "false";
    }

}

namespace Settings
{

CategorySettingValueMap Manager::mDefaultSettings = CategorySettingValueMap();
CategorySettingValueMap Manager::mUserSettings = CategorySettingValueMap();
CategorySettingVector Manager::mChangedSettings = CategorySettingVector();


class SettingsFileParser
{
public:
    SettingsFileParser() : mLine(0) {}

    void loadSettingsFile (const std::string& file, CategorySettingValueMap& settings)
    {
        mFile = file;
        boost::filesystem::ifstream stream;
        stream.open(boost::filesystem::path(file));
        std::string currentCategory;
        mLine = 0;
        while (!stream.eof() && !stream.fail())
        {
            ++mLine;
            std::string line;
            std::getline( stream, line );

            size_t i = 0;
            if (!skipWhiteSpace(i, line))
                continue;

            if (line[i] == '#') // skip comment
                continue;

            if (line[i] == '[')
            {
                size_t end = line.find(']', i);
                if (end == std::string::npos)
                    fail("unterminated category");

                currentCategory = line.substr(i+1, end - (i+1));
                boost::algorithm::trim(currentCategory);
                i = end+1;
            }

            if (!skipWhiteSpace(i, line))
                continue;

            if (currentCategory.empty())
                fail("empty category name");

            size_t settingEnd = line.find('=', i);
            if (settingEnd == std::string::npos)
                fail("unterminated setting name");

            std::string setting = line.substr(i, (settingEnd-i));
            boost::algorithm::trim(setting);

            size_t valueBegin = settingEnd+1;
            std::string value = line.substr(valueBegin);
            boost::algorithm::trim(value);

            if (settings.insert(std::make_pair(std::make_pair(currentCategory, setting), value)).second == false)
                fail(std::string("duplicate setting: [" + currentCategory + "] " + setting));
        }
    }

private:
    /// Increment i until it longer points to a whitespace character
    /// in the string or has reached the end of the string.
    /// @return false if we have reached the end of the string
    bool skipWhiteSpace(size_t& i, std::string& str)
    {
        while (i < str.size() && std::isspace(str[i], std::locale::classic()))
        {
            ++i;
        }
        return i < str.size();
    }

    void fail(const std::string& message)
    {
        std::stringstream error;
        error << "Error on line " << mLine << " in " << mFile << ":\n" << message;
        throw std::runtime_error(error.str());
    }

    std::string mFile;
    int mLine;
};

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
    boost::filesystem::ofstream stream;
    stream.open(boost::filesystem::path(file));
    std::string currentCategory;
    for (CategorySettingValueMap::iterator it = mUserSettings.begin(); it != mUserSettings.end(); ++it)
    {
        if (it->first.first != currentCategory)
        {
            currentCategory = it->first.first;
            stream << "\n[" << currentCategory << "]\n";
        }
        stream << it->first.second << " = " << it->second << "\n";
    }
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
                             + ".\nMake sure the settings-default.cfg file file was properly installed.");
}

float Manager::getFloat (const std::string& setting, const std::string& category)
{
    return parseFloat( getString(setting, category) );
}

int Manager::getInt (const std::string& setting, const std::string& category)
{
    return parseInt( getString(setting, category) );
}

bool Manager::getBool (const std::string& setting, const std::string& category)
{
    return parseBool( getString(setting, category) );
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
    setString(setting, category, toString(value));
}

void Manager::setFloat (const std::string &setting, const std::string &category, const float value)
{
    setString(setting, category, toString(value));
}

void Manager::setBool(const std::string &setting, const std::string &category, const bool value)
{
    setString(setting, category, toString(value));
}

const CategorySettingVector Manager::apply()
{
    CategorySettingVector vec = mChangedSettings;
    mChangedSettings.clear();
    return vec;
}

}
