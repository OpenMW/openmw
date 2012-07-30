#include "ogreplugin.hpp"

#include <OgrePrerequisites.h>
#include <OgreRoot.h>

#include <boost/filesystem.hpp>

namespace Files {

bool loadOgrePlugin(const std::string &pluginDir, std::string pluginName, Ogre::Root &ogreRoot) {
	pluginName = pluginName + OGRE_PLUGIN_DEBUG_SUFFIX;
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
	std::ostringstream verStream;
	verStream << "." << OGRE_VERSION_MAJOR << "." << OGRE_VERSION_MINOR << "." << OGRE_VERSION_PATCH;
	pluginName = pluginName + verStream.str();
#endif

	std::string pluginExt;
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
    pluginExt = ".dll";
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
    pluginExt = ".dylib";
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
    pluginExt = ".so";
#endif

    std::string pluginPath = pluginDir + "/" + pluginName + pluginExt;
    if (boost::filesystem::exists(pluginPath)) {
    	ogreRoot.loadPlugin(pluginPath);
    	return true;
    }
    else {
    	return false;
    }
}

}