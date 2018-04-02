#include "macospath.hpp"

#if defined(macintosh) || defined(Macintosh) || defined(__APPLE__) || defined(__MACH__)

#include <cstdlib>
#include <pwd.h>
#include <unistd.h>
#include <fstream>

#include <components/misc/stringops.hpp>

namespace
{
    sfs::path getUserHome()
    {
        const char* dir = getenv("HOME");
        if (dir == NULL)
        {
            struct passwd* pwd = getpwuid(getuid());
            if (pwd != NULL)
            {
                dir = pwd->pw_dir;
            }
        }
        if (dir == NULL)
            return sfs::path();
        else
            return sfs::path(dir);
    }
}

namespace Files
{

MacOsPath::MacOsPath(const std::string& application_name)
    : mName(application_name)
{
}

sfs::path MacOsPath::getUserConfigPath() const
{
    sfs::path userPath (getUserHome());
    userPath /= "Library/Preferences/";

    return userPath / mName;
}

sfs::path MacOsPath::getUserDataPath() const
{
    sfs::path userPath (getUserHome());
    userPath /= "Library/Application Support/";

    return userPath / mName;
}

sfs::path MacOsPath::getGlobalConfigPath() const
{
    sfs::path globalPath("/Library/Preferences/");
    return globalPath / mName;
}

sfs::path MacOsPath::getCachePath() const
{
    sfs::path userPath (getUserHome());
    userPath /= "Library/Caches";
    return userPath / mName;
}

sfs::path MacOsPath::getLocalPath() const
{
    return sfs::path("../Resources/");
}

sfs::path MacOsPath::getGlobalDataPath() const
{
    sfs::path globalDataPath("/Library/Application Support/");
    return globalDataPath / mName;
}

sfs::path MacOsPath::getInstallPath() const
{
    sfs::path installPath;

    sfs::path homePath = getUserHome();

    if (!homePath.empty())
    {
        sfs::path wineDefaultRegistry(homePath);
        wineDefaultRegistry /= ".wine/system.reg";

        if (sfs::is_regular_file(wineDefaultRegistry))
        {
            sfs::ifstream file(wineDefaultRegistry);
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
                mwpath[0] = Misc::StringUtils::toLower(mwpath[0]);
                installPath /= homePath;
                installPath /= ".wine/dosdevices/";
                installPath /= mwpath;

                if (!sfs::is_directory(installPath))
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
