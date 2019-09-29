#ifndef COMPONENTS_SETTINGS_H
#define COMPONENTS_SETTINGS_H

#include "categories.hpp"

#include <set>
#include <map>
#include <string>

namespace Settings
{
    ///
    /// \brief Settings management (can change during runtime)
    ///
    class Manager
    {
    public:
        static CategorySettingValueMap mDefaultSettings;
        static CategorySettingValueMap mUserSettings;

        static CategorySettingVector mChangedSettings;
        ///< tracks all the settings that were changed since the last apply() call

        void clear();
        ///< clears all settings and default settings

        void loadDefault (const std::string& file);
        ///< load file as the default settings (can be overridden by user settings)

        void loadUser (const std::string& file);
        ///< load file as user settings

        void saveUser (const std::string& file);
        ///< save user settings to file

        static void resetPendingChange(const std::string &setting, const std::string &category);

        static void resetPendingChanges();

        static const CategorySettingVector getPendingChanges();
        ///< returns the list of changed settings and then clears it

        static int getInt (const std::string& setting, const std::string& category);
        static float getFloat (const std::string& setting, const std::string& category);
        static std::string getString (const std::string& setting, const std::string& category);
        static bool getBool (const std::string& setting, const std::string& category);

        static void setInt (const std::string& setting, const std::string& category, const int value);
        static void setFloat (const std::string& setting, const std::string& category, const float value);
        static void setString (const std::string& setting, const std::string& category, const std::string& value);
        static void setBool (const std::string& setting, const std::string& category, const bool value);
    };

}

#endif // _COMPONENTS_SETTINGS_H
