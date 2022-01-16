#ifndef COMPONENTS_SETTINGS_H
#define COMPONENTS_SETTINGS_H

#include "categories.hpp"

#include <set>
#include <map>
#include <string>
#include <osg/Vec2f>
#include <osg/Vec3f>

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
        ///< resets a single pending change

        static void resetPendingChanges();
        ///< resets the list of all pending changes

        static void resetPendingChanges(const CategorySettingVector& filter);
        ///< resets only the pending changes listed in the filter

        static CategorySettingVector getPendingChanges();
        ///< returns the list of changed settings

        static CategorySettingVector getPendingChanges(const CategorySettingVector& filter);
        ///< returns the list of changed settings intersecting with the filter

        static int getInt (const std::string& setting, const std::string& category);
        static std::int64_t getInt64 (const std::string& setting, const std::string& category);
        static float getFloat (const std::string& setting, const std::string& category);
        static double getDouble (const std::string& setting, const std::string& category);
        static std::string getString (const std::string& setting, const std::string& category);
        static bool getBool (const std::string& setting, const std::string& category);
        static osg::Vec2f getVector2 (const std::string& setting, const std::string& category);
        static osg::Vec3f getVector3 (const std::string& setting, const std::string& category);

        static void setInt (const std::string& setting, const std::string& category, int value);
        static void setFloat (const std::string& setting, const std::string& category, float value);
        static void setDouble (const std::string& setting, const std::string& category, double value);
        static void setString (const std::string& setting, const std::string& category, const std::string& value);
        static void setBool (const std::string& setting, const std::string& category, bool value);
        static void setVector2 (const std::string& setting, const std::string& category, osg::Vec2f value);
        static void setVector3 (const std::string& setting, const std::string& category, osg::Vec3f value);
    };

}

#endif // COMPONENTS_SETTINGS_H
