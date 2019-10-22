#ifndef COMPONENTS_SETTINGS_PARSER_H
#define COMPONENTS_SETTINGS_PARSER_H

#include "categories.hpp"

#include <string>

namespace Settings
{
    class SettingsFileParser
    {
    public:
        void loadSettingsFile(const std::string& file, CategorySettingValueMap& settings);

        void saveSettingsFile(const std::string& file, const CategorySettingValueMap& settings);

    private:
        /// Increment i until it longer points to a whitespace character
        /// in the string or has reached the end of the string.
        /// @return false if we have reached the end of the string
        bool skipWhiteSpace(size_t& i, std::string& str);

        void fail(const std::string& message);

        std::string mFile;
        int mLine = 0;
    };
}

#endif // _COMPONENTS_SETTINGS_PARSER_H
