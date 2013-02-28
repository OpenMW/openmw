#ifndef COMPONENTS_FILES_OGREPLUGIN_H
#define COMPONENTS_FILES_OGREPLUGIN_H

#include <string>

#include <boost/filesystem.hpp>
#include <boost/version.hpp>

namespace Ogre {
	class Root;
}

#if (BOOST_VERSION <= 104500)
namespace boost {
namespace filesystem {
inline path absolute(const path& p, const path& base=current_path()) {
	// call obsolete version of this function on older boost
	return complete(p, base);
}
}
}
#endif /* (BOOST_VERSION <= 104300) */

/**
 * \namespace Files
 */
namespace Files {

/**
 * \brief Loads Ogre plugin with given name.
 *
 * \param pluginDir absolute path to plugins
 * \param pluginName plugin name, for example "RenderSystem_GL"
 * \param ogreRoot Ogre::Root instance
 *
 * \return whether plugin was located or not
 */
bool loadOgrePlugin(const std::string &pluginDir, std::string pluginName, Ogre::Root &ogreRoot);

}

#endif /* COMPONENTS_FILES_OGREPLUGIN_H */
