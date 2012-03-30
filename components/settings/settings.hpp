#ifndef _COMPONENTS_SETTINGS_H
#define _COMPONENTS_SETTINGS_H

#include <OgreConfigFile.h>

namespace Settings
{

    ///
    /// \brief Settings management (can change during runtime)
    ///
    class Manager
    {
    public:
        static Ogre::ConfigFile mFile;
        static Ogre::ConfigFile mDefaultFile;

        void loadDefault (const std::string& file);
        ///< load file as the default settings (can be overridden by user settings)

        void load (const std::string& file);
        ///< load file as user settings

        void save (const std::string& file);
        ///< save to file

        static const int getInt (const std::string& setting, const std::string& category);
        static const float getFloat (const std::string& setting, const std::string& category);
        static const std::string getString (const std::string& setting, const std::string& category);
        static const bool getBool (const std::string& setting, const std::string& category);
    };

}

#endif // _COMPONENTS_SETTINGS_H
