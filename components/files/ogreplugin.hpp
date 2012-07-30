/**
 *  Open Morrowind - an opensource Elder Scrolls III: Morrowind
 *  engine implementation.
 *
 *  Copyright (C) 2011 Open Morrowind Team
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/** \file components/files/ogreplugin.hpp */

#ifndef COMPONENTS_FILES_OGREPLUGIN_H
#define COMPONENTS_FILES_OGREPLUGIN_H

#include <string>

namespace Ogre {
	class Root;
}

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