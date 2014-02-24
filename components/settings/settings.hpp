#ifndef COMPONENTS_SETTINGS_H
#define COMPONENTS_SETTINGS_H

#include <OgreConfigFile.h>

namespace Settings
{
    typedef std::pair < std::string, std::string > CategorySetting; 
    typedef std::vector< std::pair<std::string, std::string> > CategorySettingVector;
    typedef std::map < CategorySetting, std::string > CategorySettingValueMap;

    ///
    /// \brief Settings management (can change during runtime)
    ///
    class Manager
    {
    public:
        static Ogre::ConfigFile mFile;
        static Ogre::ConfigFile mDefaultFile;

        static CategorySettingVector mChangedSettings;
        ///< tracks all the settings that were changed since the last apply() call

        static CategorySettingValueMap mNewSettings;
        ///< tracks all the settings that are in the default file, but not in user file yet

        void loadDefault (const std::string& file);
        ///< load file as the default settings (can be overridden by user settings)

        void loadUser (const std::string& file);
        ///< load file as user settings

        void saveUser (const std::string& file);
        ///< save user settings to file

        static const CategorySettingVector apply();
        ///< returns the list of changed settings and then clears it

        static const int getInt (const std::string& setting, const std::string& category);
        static const float getFloat (const std::string& setting, const std::string& category);
        static const std::string getString (const std::string& setting, const std::string& category);
        static const bool getBool (const std::string& setting, const std::string& category);

        static void setInt (const std::string& setting, const std::string& category, const int value);
        static void setFloat (const std::string& setting, const std::string& category, const float value);
        static void setString (const std::string& setting, const std::string& category, const std::string& value);
        static void setBool (const std::string& setting, const std::string& category, const bool value);
    };

}

#endif // _COMPONENTS_SETTINGS_H
