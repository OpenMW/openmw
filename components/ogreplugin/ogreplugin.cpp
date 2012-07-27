#include "ogreplugin.h"

#include <OgrePrerequisites.h>
#include <OgreRoot.h>

#include <boost/filesystem.hpp>

bool loadOgrePlugin(std::string pluginDir, std::string pluginName, Ogre::Root &ogreRoot) {
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
	std::ostringstream verStream;
	verStream << "." << OGRE_VERSION_MAJOR << "." << OGRE_VERSION_MINOR << "." << OGRE_VERSION_PATCH;
	pluginName = pluginName + OGRE_PLUGIN_DEBUG_SUFFIX + verStream.str();
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

    std::cout << "loading plugin: " << pluginPath << std::endl;

    if (boost::filesystem::exists(pluginPath)) {
    	ogreRoot.loadPlugin(pluginPath);
    	return true;
    }
    else {
    	return false;
    }
}