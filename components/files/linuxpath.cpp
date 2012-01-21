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

/** \file components/files/linuxpath.cpp */

#include "linuxpath.hpp"

#if defined(__linux__) || defined(__FreeBSD__)

#include <cstdlib>
#include <cstring>
#include <pwd.h>
#include <unistd.h>

/**
 * \namespace Files
 */
namespace Files
{

boost::filesystem::path LinuxPath::getUserPath() const
{
    boost::filesystem::path userPath(".");
    boost::filesystem::path suffix("/");

    const char* theDir = getenv("OPENMW_CONFIG");
    if (theDir == NULL)
    {
        theDir = getenv("XDG_CONFIG_HOME");
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
                suffix = boost::filesystem::path("/.config/");
            }
        }
    }

    if (theDir != NULL) {
        userPath = boost::filesystem::path(theDir);
    }

    userPath /= suffix;

    return userPath;
}

boost::filesystem::path LinuxPath::getGlobalPath() const
{
    boost::filesystem::path globalPath("/etc/xdg/");

    char* theDir = getenv("XDG_CONFIG_DIRS");
    if (theDir != NULL)
    {
        // We take only first path from list
        char* ptr = strtok(theDir, ":");
        if (ptr != NULL)
        {
            globalPath = boost::filesystem::path(ptr);
            globalPath /= boost::filesystem::path("/");
        }
    }

    return globalPath;
}

boost::filesystem::path LinuxPath::getLocalPath() const
{
    return boost::filesystem::path("./");
}

boost::filesystem::path LinuxPath::getUserDataPath() const
{
    boost::filesystem::path localDataPath(".");
    boost::filesystem::path suffix("/");

    const char* theDir = getenv("OPENMW_DATA");
    if (theDir == NULL)
    {
        theDir = getenv("XDG_DATA_HOME");
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
                suffix = boost::filesystem::path("/.local/share/");
            }
        }
    }

    if (theDir != NULL) {
        localDataPath = boost::filesystem::path(theDir);
    }

    localDataPath /= suffix;
    return localDataPath;
}

boost::filesystem::path LinuxPath::getGlobalDataPath() const
{
    boost::filesystem::path globalDataPath("/usr/local/share/");

    char* theDir = getenv("XDG_DATA_DIRS");
    if (theDir != NULL)
    {
        // We take only first path from list
        char* ptr = strtok(theDir, ":");
        if (ptr != NULL)
        {
            globalDataPath = boost::filesystem::path(ptr);
            globalDataPath /= boost::filesystem::path("/");
        }
    }

    return globalDataPath;
}

boost::filesystem::path LinuxPath::getLocalDataPath() const
{
    return boost::filesystem::path("./data/");
}


boost::filesystem::path LinuxPath::getInstallPath() const
{
    return boost::filesystem::path("./");
}

} /* namespace Files */

#endif /* defined(__linux__) || defined(__FreeBSD__) */
