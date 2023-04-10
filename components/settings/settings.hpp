#ifndef COMPONENTS_SETTINGS_H
#define COMPONENTS_SETTINGS_H

#include "categories.hpp"

#include <set>
#include <string>
#include <string_view>
#include <vector>

#include <filesystem>
#include <osg/Vec2f>
#include <osg/Vec3f>

namespace Files
{
    struct ConfigurationManager;
}

namespace Settings
{
    enum class WindowMode
    {
        Fullscreen = 0,
        WindowedFullscreen,
        Windowed
    };

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

        static void clear();
        ///< clears all settings and default settings

        static std::filesystem::path load(const Files::ConfigurationManager& cfgMgr, bool loadEditorSettings = false);
        ///< load settings from all active config dirs. Returns the path of the last loaded file.

        static void saveUser(const std::filesystem::path& file);
        ///< save user settings to file

        static void resetPendingChanges();
        ///< resets the list of all pending changes

        static void resetPendingChanges(const CategorySettingVector& filter);
        ///< resets only the pending changes listed in the filter

        static CategorySettingVector getPendingChanges();
        ///< returns the list of changed settings

        static CategorySettingVector getPendingChanges(const CategorySettingVector& filter);
        ///< returns the list of changed settings intersecting with the filter

        static int getInt(std::string_view setting, std::string_view category);
        static std::uint64_t getUInt64(std::string_view setting, std::string_view category);
        static std::size_t getSize(std::string_view setting, std::string_view category);
        static unsigned getUnsigned(std::string_view setting, std::string_view category);
        static unsigned long getUnsignedLong(std::string_view setting, std::string_view category);
        static unsigned long long getUnsignedLongLong(std::string_view setting, std::string_view category);
        static float getFloat(std::string_view setting, std::string_view category);
        static double getDouble(std::string_view setting, std::string_view category);
        static const std::string& getString(std::string_view setting, std::string_view category);
        static std::vector<std::string> getStringArray(std::string_view setting, std::string_view category);
        static bool getBool(std::string_view setting, std::string_view category);
        static osg::Vec2f getVector2(std::string_view setting, std::string_view category);
        static osg::Vec3f getVector3(std::string_view setting, std::string_view category);

        template <class T>
        static T get(std::string_view setting, std::string_view category)
        {
            recordInit(setting, category);
            return getImpl<T>(setting, category);
        }

        static void setInt(std::string_view setting, std::string_view category, int value);
        static void setUInt64(std::string_view setting, std::string_view category, std::uint64_t value);
        static void setFloat(std::string_view setting, std::string_view category, float value);
        static void setDouble(std::string_view setting, std::string_view category, double value);
        static void setString(std::string_view setting, std::string_view category, const std::string& value);
        static void setStringArray(
            std::string_view setting, std::string_view category, const std::vector<std::string>& value);
        static void setBool(std::string_view setting, std::string_view category, bool value);
        static void setVector2(std::string_view setting, std::string_view category, osg::Vec2f value);
        static void setVector3(std::string_view setting, std::string_view category, osg::Vec3f value);

        static void set(std::string_view setting, std::string_view category, int value);
        static void set(std::string_view setting, std::string_view category, unsigned value);
        static void set(std::string_view setting, std::string_view category, unsigned long value);
        static void set(std::string_view setting, std::string_view category, unsigned long long value);
        static void set(std::string_view setting, std::string_view category, float value);
        static void set(std::string_view setting, std::string_view category, double value);
        static void set(std::string_view setting, std::string_view category, const std::string& value);
        static void set(std::string_view setting, std::string_view category, bool value);
        static void set(std::string_view setting, std::string_view category, const osg::Vec2f& value);
        static void set(std::string_view setting, std::string_view category, const osg::Vec3f& value);

    private:
        static std::set<std::pair<std::string_view, std::string_view>> sInitialized;

        template <class T>
        static T getImpl(std::string_view setting, std::string_view category);

        static void recordInit(std::string_view setting, std::string_view category);
    };

    template <>
    inline int Manager::getImpl<int>(std::string_view setting, std::string_view category)
    {
        return getInt(setting, category);
    }

    template <>
    inline unsigned Manager::getImpl<unsigned>(std::string_view setting, std::string_view category)
    {
        return getUnsigned(setting, category);
    }

    template <>
    inline unsigned long Manager::getImpl<unsigned long>(std::string_view setting, std::string_view category)
    {
        return getUnsignedLong(setting, category);
    }

    template <>
    inline unsigned long long Manager::getImpl<unsigned long long>(std::string_view setting, std::string_view category)
    {
        return getUnsignedLongLong(setting, category);
    }

    template <>
    inline float Manager::getImpl<float>(std::string_view setting, std::string_view category)
    {
        return getFloat(setting, category);
    }

    template <>
    inline double Manager::getImpl<double>(std::string_view setting, std::string_view category)
    {
        return getDouble(setting, category);
    }

    template <>
    inline std::string Manager::getImpl<std::string>(std::string_view setting, std::string_view category)
    {
        return getString(setting, category);
    }

    template <>
    inline bool Manager::getImpl<bool>(std::string_view setting, std::string_view category)
    {
        return getBool(setting, category);
    }

    template <>
    inline osg::Vec2f Manager::getImpl<osg::Vec2f>(std::string_view setting, std::string_view category)
    {
        return getVector2(setting, category);
    }

    template <>
    inline osg::Vec3f Manager::getImpl<osg::Vec3f>(std::string_view setting, std::string_view category)
    {
        return getVector3(setting, category);
    }
}

#endif // COMPONENTS_SETTINGS_H
