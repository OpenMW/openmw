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
#include <boost/filesystem/fstream.hpp>

/**
 * FIXME: Someone with MacOS system should check this and correct if necessary
 */

/**
 * \namespace Files
 */
namespace Files
{

boost::filesystem::path MacOsPath::getUserPath() const
{
    boost::filesystem::path userPath(".");
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
        userPath = boost::filesystem::path(theDir) / "Library/Preferences/";
    }

    userPath /= suffix;

    return userPath;
}

boost::filesystem::path MacOsPath::getGlobalPath() const
{
    boost::filesystem::path globalPath("/Library/Preferences/");
    return globalPath;
}

boost::filesystem::path MacOsPath::getCachePath() const
{
    boost::filesystem::path userPath(".");

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
        userPath = boost::filesystem::path(theDir) / "Library/Caches";
    }

    return userPath;
}

boost::filesystem::path MacOsPath::getLocalPath() const
{
    return boost::filesystem::path("./");
}

boost::filesystem::path MacOsPath::getGlobalDataPath() const
{
    boost::filesystem::path globalDataPath("/Library/Application Support/");
    return globalDataPath;
}

boost::filesystem::path MacOsPath::getInstallPath() const
{
    boost::filesystem::path installPath;

    char *homePath = getenv("HOME");
    if (homePath == NULL)
    {
        struct passwd* pwd = getpwuid(getuid());
        if (pwd != NULL)
        {
            homePath = pwd->pw_dir;
        }
    }

    if (homePath != NULL)
    {
        boost::filesystem::path wineDefaultRegistry(homePath);
        wineDefaultRegistry /= ".wine/system.reg";

        if (boost::filesystem::is_regular_file(wineDefaultRegistry))
        {
            boost::filesystem::ifstream file(wineDefaultRegistry);
            bool isRegEntry = false;
            std::string line;
            std::string mwpath;

            while (std::getline(file, line))
            {
                if (line[0] == '[') // we found an entry
                {
                    if (isRegEntry)
                    {
                        break;
                    }

                    isRegEntry = (line.find("Softworks\\\\Morrowind]") != std::string::npos);
                }
                else if (isRegEntry)
                {
                    if (line[0] == '"') // empty line means new registry key
                    {
                        std::string key = line.substr(1, line.find('"', 1) - 1);
                        if (strcasecmp(key.c_str(), "Installed Path") == 0)
                        {
                            std::string::size_type valuePos = line.find('=') + 2;
                            mwpath = line.substr(valuePos, line.rfind('"') - valuePos);

                            std::string::size_type pos = mwpath.find("\\");
                            while (pos != std::string::npos)
                            {
                               mwpath.replace(pos, 2, "/");
                               pos = mwpath.find("\\", pos + 1);
                            }
                            break;
                        }
                    }
                }
            }

            if (!mwpath.empty())
            {
                // Change drive letter to lowercase, so we could use ~/.wine/dosdevice symlinks
                mwpath[0] = tolower(mwpath[0]);
                installPath /= homePath;
                installPath /= ".wine/dosdevices/";
                installPath /= mwpath;

                if (!boost::filesystem::is_directory(installPath))
                {
                    installPath.clear();
                }
            }
        }
    }

    return installPath;
}


} /* namespace Files */

#endif /* defined(macintosh) || defined(Macintosh) || defined(__APPLE__) || defined(__MACH__) */
