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

/** \file components/files/macospath.cpp */

#include "macospath.hpp"

#if defined(macintosh) || defined(Macintosh) || defined(__APPLE__) || defined(__MACH__)

#include <cstdlib>
#include <pwd.h>
#include <unistd.h>

/**
 * \namespace Files
 */
namespace Files
{

boost::filesystem::path MacOsPath::getLocalConfigPath() const
{
    boost::filesystem::path localConfigPath(".");
    boost::filesystem::path suffix("/");

    const char* theDir = getenv("HOME");
    if (theDir == NULL)
    {
        struct passwd* pwd = getpwuid(getuid());
        if (pwd != NULL)
        {
            theDir = pwd->pw_dir;
        }
    }
    if (theDir != NULL)
    {
        localConfigPath = boost::filesystem::path(theDir) / "Library/Preferences/";
    }

    localConfigPath /= suffix;

    return localConfigPath;
}

boost::filesystem::path MacOsPath::getGlobalConfigPath() const
{
    boost::filesystem::path globalConfigPath("/Library/Preferences/");
    return globalConfigPath;
}

boost::filesystem::path MacOsPath::getRuntimeConfigPath() const
{
    return boost::filesystem::path("./");
}

boost::filesystem::path MacOsPath::getLocalDataPath() const
{
    boost::filesystem::path localDataPath(".");
    boost::filesystem::path suffix("/");

    const char* theDir = getenv("OPENMW_DATA");
    if (theDir == NULL)
    {
        theDir = getenv("HOME");
        if (theDir == NULL)
        {
            struct passwd* pwd = getpwuid(getuid());
            if (pwd != NULL)
            {
                theDir = pwd->pw_dir;
            }
        }
        if (theDir != NULL)
        {
            suffix = boost::filesystem::path("/Library/Application Support/");
        }
    }

    if (theDir != NULL)
    {
        localDataPath = boost::filesystem::path(theDir);
    }

    localDataPath /= suffix;
    return localDataPath;
}

boost::filesystem::path MacOsPath::getGlobalDataPath() const
{
    boost::filesystem::path globalDataPath("/Library/Application Support/");
    return globalDataPath;
}

boost::filesystem::path MacOsPath::getRuntimeDataPath() const
{
    return boost::filesystem::path("./data/");
}


} /* namespace Files */

#endif /* defined(macintosh) || defined(Macintosh) || defined(__APPLE__) || defined(__MACH__) */
