#include "settings.hpp"

#include <sstream>
#include <iostream>

#include <components/misc/stringops.hpp>

#include <boost/filesystem/fstream.hpp>
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

typedef std::map< CategorySetting, bool > CategorySettingStatusMap;

class SettingsFileParser
{
public:
    SettingsFileParser() : mLine(0) {}

    void loadSettingsFile (const std::string& file, CategorySettingValueMap& settings)
    {
        mFile = file;
        boost::filesystem::ifstream stream;
        stream.open(boost::filesystem::path(file));
        std::cout << "Loading settings file: " << file << std::endl;
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

    void saveSettingsFile (const std::string& file, CategorySettingValueMap& settings)
    {
        // No options have been written to the file yet.
        CategorySettingStatusMap written;
        for (CategorySettingValueMap::iterator it = settings.begin(); it != settings.end(); ++it) {
            written[it->first] = false;
        }

        // Have we substantively changed the settings file?
        bool changed = false;

        // Were there any lines at all in the file?
        bool existing = false;

        // The category/section we're currently in.
        std::string currentCategory;

        // Open the existing settings.cfg file to copy comments.  This might not be the same file
        // as the output file if we're copying the setting from the default settings.cfg for the
        // first time.  A minor change in API to pass the source file might be in order here.
        boost::filesystem::ifstream istream;
        boost::filesystem::path ipath(file);
        istream.open(ipath);

        // Create a new string stream to write the current settings to.  It's likely that the
        // input file and the output file are the same, so this stream serves as a temporary file
        // of sorts.  The setting files aren't very large so there's no performance issue.
        std::stringstream ostream;

        // For every line in the input file...
        while (!istream.eof() && !istream.fail()) {
            std::string line;
            std::getline(istream, line);

            // The current character position in the line.
            size_t i = 0;

            // Don't add additional newlines at the end of the file.
            if (istream.eof()) continue;

            // Copy entirely blank lines.
            if (!skipWhiteSpace(i, line)) {
                ostream << line << std::endl;
                continue;
            }

            // There were at least some comments in the input file.
            existing = true;

            // Copy comments.
            if (line[i] == '#') {
                ostream << line << std::endl;
                continue;
            }

            // Category heading.
            if (line[i] == '[') {
                size_t end = line.find(']', i);
                // This should never happen unless the player edited the file while playing.
                if (end == std::string::npos) {
                    ostream << "# unterminated category: " << line << std::endl;
                    changed = true;
                    continue;
                }

                // Ensure that all options in the current category have been written.
                for (CategorySettingStatusMap::iterator mit = written.begin(); mit != written.end(); ++mit) {
                    if (mit->second == false && mit->first.first == currentCategory) {
                        std::cout << "Added new setting: [" << currentCategory << "] "
                                  << mit->first.second << " = " << settings[mit->first] << std::endl;
                        ostream << mit->first.second << " = " << settings[mit->first] << std::endl;
                        mit->second = true;
                        changed = true;
                    }
                }

                // Update the current category.
                currentCategory = line.substr(i+1, end - (i+1));
                boost::algorithm::trim(currentCategory);

                // Write the (new) current category to the file.
                ostream << "[" << currentCategory << "]" << std::endl;
                //std::cout << "Wrote category: " << currentCategory << std::endl;

                // A setting can apparently follow the category on an input line.  That's rather
                // inconvenient, since it makes it more likely to have duplicative sections,
                // which our algorithm doesn't like.  Do the best we can.
                i = end+1;
            }

            // Truncate trailing whitespace, since we're changing the file anayway.
            if (!skipWhiteSpace(i, line))
                continue;

            // If we've found settings before the first category, something's wrong.  This
            // should never happen unless the player edited the file while playing, since
            // the loadSettingsFile() logic rejects it.
            if (currentCategory.empty()) {
                ostream << "# empty category name: " << line << std::endl;
                changed = true;
                continue;
            }

            // Which setting was at this location in the input file?
            size_t settingEnd = line.find('=', i);
            // This should never happen unless the player edited the file while playing.
            if (settingEnd == std::string::npos) {
                ostream << "# unterminated setting name: " << line << std::endl;
                changed = true;
                continue;
            }
            std::string setting = line.substr(i, (settingEnd-i));
            boost::algorithm::trim(setting);

            // Get the existing value so we can see if we've changed it.
            size_t valueBegin = settingEnd+1;
            std::string value = line.substr(valueBegin);
            boost::algorithm::trim(value);

            // Construct the setting map key to determine whether the setting has already been
            // written to the file.
            CategorySetting key = std::make_pair(currentCategory, setting);
            CategorySettingStatusMap::iterator finder = written.find(key);

            // Settings not in the written map are definitely invalid.  Currently, this can only
            // happen if the player edited the file while playing, because loadSettingsFile()
            // will accept anything and pass it along in the map, but in the future, we might
            // want to handle invalid settings more gracefully here.
            if (finder == written.end()) {
                ostream << "# invalid setting: " << line << std::endl;
                changed = true;
                continue;
            }

            // Write the current value of the setting to the file.  The key must exist in the
            // settings map because of how written was initialized and finder != end().
            ostream << setting << " = " << settings[key] << std::endl;
            // Mark that setting as written.
            finder->second = true;
            // Did we really change it?
            if (value != settings[key]) {
                std::cout << "Changed setting: [" << currentCategory << "] "
                          << setting << " = " << settings[key] << std::endl;
                changed = true;
            }
            // No need to write the current line, because we just emitted a replacement.

            // Curiously, it appears that comments at the ends of lines with settings are not
            // allowed, and the comment becomes part of the value.  Was that intended?
        }

        // We're done with the input stream file.
        istream.close();

        // Ensure that all options in the current category have been written.  We must complete
        // the current category at the end of the file before moving on to any new categories.
        for (CategorySettingStatusMap::iterator mit = written.begin(); mit != written.end(); ++mit) {
            if (mit->second == false && mit->first.first == currentCategory) {
                std::cout << "Added new setting: [" << mit->first.first << "] "
                          << mit->first.second << " = " << settings[mit->first] << std::endl;
                ostream << mit->first.second << " = " << settings[mit->first] << std::endl;
                mit->second = true;
                changed = true;
            }
        }

        // If there was absolutely nothing in the file (or more likely the file didn't
        // exist), start the newly created file with a helpful comment.
        if (!existing) {
            ostream << "# This is the OpenMW user 'settings.cfg' file.  This file only contains" << std::endl;
            ostream << "# explicitly changed settings.  If you would like to revert a setting" << std::endl;
            ostream << "# to its default, simply remove it from this file.  For available" << std::endl;
            ostream << "# settings, see the file 'settings-default.cfg' or the documentation at:" << std::endl;
            ostream << "#" << std::endl;
            ostream << "#   https://openmw.readthedocs.io/en/master/reference/modding/settings/index.html" << std::endl;
        }

        // We still have one more thing to do before we're completely done writing the file.
        // It's possible that there are entirely new categories, or that the input file had
        // disappeared completely, so we need ensure that all settings are written to the file
        // regardless of those issues.
        for (CategorySettingStatusMap::iterator mit = written.begin(); mit != written.end(); ++mit) {
            // If the setting hasn't been written yet.
            if (mit->second == false) {
                // If the catgory has changed, write a new category header.
                if (mit->first.first != currentCategory) {
                    currentCategory = mit->first.first;
                    std::cout << "Created new setting section: " << mit->first.first << std::endl;
                    ostream << std::endl;
                    ostream << "[" << currentCategory << "]" << std::endl;
                }
                std::cout << "Added new setting: [" << mit->first.first << "] "
                          << mit->first.second << " = " << settings[mit->first] << std::endl;
                // Then write the setting.  No need to mark it as written because we're done.
                ostream << mit->first.second << " = " << settings[mit->first] << std::endl;
                changed = true;
            }
        }

        // Now install the newly written file in the requested place.
        if (changed) {
            std::cout << "Updating settings file: " << ipath << std::endl;
            boost::filesystem::ofstream ofstream;
            ofstream.open(ipath);
            ofstream << ostream.rdbuf();
            ofstream.close();
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
