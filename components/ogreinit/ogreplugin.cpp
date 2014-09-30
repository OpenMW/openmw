#include "ogreplugin.hpp"

#include <OgrePrerequisites.h>
#include <OgreRoot.h>

namespace Files {

bool loadOgrePlugin(const std::string &pluginDir, std::string pluginName, Ogre::Root &ogreRoot) {
	std::string pluginExt;
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
    pluginExt = ".dll";
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
    pluginExt = ".framework";
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
    pluginExt = ".so";
#endif

    // Append plugin suffix if debugging.
    std::string pluginPath;
#if defined(DEBUG)
    pluginPath = pluginDir + "/" + pluginName + OGRE_PLUGIN_DEBUG_SUFFIX + pluginExt;
    if (boost::filesystem::exists(pluginPath)) {
        ogreRoot.loadPlugin(pluginPath);
        return true;
    }
    else {
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
        return false;
#endif //OGRE_PLATFORM == OGRE_PLATFORM_WIN32
    }
#endif //defined(DEBUG)
    
    pluginPath = pluginDir + "/" + pluginName + pluginExt;
    if (boost::filesystem::exists(pluginPath)) {
        ogreRoot.loadPlugin(pluginPath);
        return true;
    }
    else {
        return false;
    }
}

}
