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

#if defined(__linux__)

#include <cstdlib>
#include <cstring>
#include <pwd.h>
#include <unistd.h>
#include <boost/filesystem/fstream.hpp>

/**
 * \namespace Files
 */
namespace Files
{

boost::filesystem::path LinuxPath::getLocalConfigPath() const
{
    boost::filesystem::path localConfigPath(".");
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
        localConfigPath = boost::filesystem::path(theDir);
    }

    localConfigPath /= suffix;

    return localConfigPath;
}

boost::filesystem::path LinuxPath::getGlobalConfigPath() const
{
    boost::filesystem::path globalConfigPath("/etc/xdg/");

    char* theDir = getenv("XDG_CONFIG_DIRS");
    if (theDir != NULL)
    {
        // We take only first path from list
        char* ptr = strtok(theDir, ":");
        if (ptr != NULL)
        {
            globalConfigPath = boost::filesystem::path(ptr);
            globalConfigPath /= boost::filesystem::path("/");
        }
    }

    return globalConfigPath;
}

boost::filesystem::path LinuxPath::getRuntimeConfigPath() const
{
    return boost::filesystem::path("./");
}

boost::filesystem::path LinuxPath::getLocalDataPath() const
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

boost::filesystem::path LinuxPath::getRuntimeDataPath() const
{
    return boost::filesystem::path("./data/");
}

boost::filesystem::path LinuxPath::getInstallPath() const
{
    char *homePath = getenv("HOME");
    if(!homePath)
    {
        return boost::filesystem::path("");
    }
    
    boost::filesystem::path wineDefaultRegistry(homePath);
    wineDefaultRegistry /= ".wine/system.reg";
    
    boost::filesystem::path wineDriveC(homePath);
    wineDriveC /= ".wine/drive_c";

    boost::filesystem::file_status fileStatus = boost::filesystem::status(wineDefaultRegistry);
    boost::filesystem::file_status dirStatus = boost::filesystem::status(wineDriveC);
    if(!boost::filesystem::is_regular_file(fileStatus) || !boost::filesystem::is_directory(dirStatus))
    {
        return boost::filesystem::path("");
    }
    
    
    boost::filesystem::ifstream file(wineDefaultRegistry);
    bool isRegEntry = false;
    std::string line;
    
    while (std::getline(file, line))
    {
        if(!line.empty() && line[0] == '[') // we found an entry
        {
            std::string regkey = line.substr(1, line.find(']')-1);
            if( regkey.compare("SOFTWARE\\\\Wow6432Node\\\\Bethesda Softworks\\\\Morrowind") == 0
             || regkey.compare("SOFTWARE\\\\Bethesda Softworks\\\\Morrowind") == 0 )
            {
                isRegEntry = true;
            }
        }
        else if(isRegEntry)
        {
            if(line.empty() || line[0] != '"') // empty line means new registry key
            {
                break;
            }
            std::string key = line.substr(1, line.find('"', 1)-1);
            if(key.compare("Installed Path") == 0) {
                std::string::size_type pos, startPos;
                
                startPos = line.find('=')+2;
                std::string installPath = line.substr(startPos, line.find('"', startPos+1)-startPos);
                installPath.replace(0, 2, wineDriveC.string());
                
                pos = -1;
                do
                {
                    pos = installPath.find("\\\\", pos+1);
                    if(pos == std::string::npos)
                    {
                        break;
                    }
                    
                    installPath.replace(pos, 2, "/");
                } while(true);
                
                return boost::filesystem::path(installPath);
            }
        }
    }
    
    return boost::filesystem::path("");
}


} /* namespace Files */

#endif /* defined(__linux__) */
