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

/** \file components/files/macospath.hpp */

#ifndef COMPONENTS_FILES_MACOSPATH_H
#define COMPONENTS_FILES_MACOSPATH_H

#if defined(macintosh) || defined(Macintosh) || defined(__APPLE__) || defined(__MACH__)

#include <boost/filesystem.hpp>

/**
 * \namespace Files
 */
namespace Files
{

/**
 * \struct MacOsPath
 */
struct MacOsPath
{
    /**
     * \brief Return path to the local configuration directory.
     *
     * \return boost::filesystem::path
     */
    boost::filesystem::path getUserPath() const;

    /**
     * \brief Return path to the global (system) configuration directory.
     *
     * \return boost::filesystem::path
     */
    boost::filesystem::path getGlobalPath() const;

    /**
     * \brief Return path to the runtime configuration directory which is the
     * place where an application was started.
     *
     * \return boost::filesystem::path
     */
    boost::filesystem::path getLocalPath() const;
};

} /* namespace Files */

#endif /* defined(macintosh) || defined(Macintosh) || defined(__APPLE__) || defined(__MACH__) */

#endif /* COMPONENTS_FILES_MACOSPATH_H */
