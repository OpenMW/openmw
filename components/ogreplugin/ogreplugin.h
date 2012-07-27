#ifndef OGREPLUGIN_H
#define OGREPLUGIN_H

#include <string>

namespace Ogre {
	class Root;
}

extern bool loadOgrePlugin(std::string pluginDir, std::string pluginName, Ogre::Root &ogreRoot);

#endif