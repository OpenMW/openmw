#include "linuxpath.hpp"

#if defined(__linux__) || defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__OpenBSD__)

#include <pwd.h>
#include <unistd.h>
#include <fstream>
#include <string.h>

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

    sfs::path getEnv(const std::string& envVariable, const sfs::path& fallback)
    {
        const char* result = getenv(envVariable.c_str());
        if (!result)
            return fallback;
        sfs::path dir(result);
        if (dir.empty())
            return fallback;
        else
            return dir;
    }
}

/**
 * \namespace Files
 */
namespace Files
{

LinuxPath::LinuxPath(const std::string& application_name)
    : mName(application_name)
{
}

sfs::path LinuxPath::getUserConfigPath() const
{
    return getEnv("XDG_CONFIG_HOME", getUserHome() / ".config") / mName;
}

sfs::path LinuxPath::getUserDataPath() const
{
    return getEnv("XDG_DATA_HOME", getUserHome() / ".local/share") / mName;
}

sfs::path LinuxPath::getCachePath() const
{
    return getEnv("XDG_CACHE_HOME", getUserHome() / ".cache") / mName;
}

sfs::path LinuxPath::getGlobalConfigPath() const
{
    sfs::path globalPath(GLOBAL_CONFIG_PATH);
    return globalPath / mName;
}

sfs::path LinuxPath::getLocalPath() const
{
    return sfs::path("./");
}

sfs::path LinuxPath::getGlobalDataPath() const
{
    sfs::path globalDataPath(GLOBAL_DATA_PATH);
    return globalDataPath / mName;
}

sfs::path LinuxPath::getInstallPath() const
{
    sfs::path installPath;

    sfs::path homePath = getUserHome();

    if (!homePath.empty())
    {
        sfs::path wineDefaultRegistry(homePath);
        wineDefaultRegistry /= ".wine/system.reg";

        if (sfs::is_regular_file(wineDefaultRegistry))
        {
            std::ifstream file(wineDefaultRegistry);
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
                // Change drive letter to lowercase, so we could use
                // ~/.wine/dosdevices symlinks
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

#endif /* defined(__linux__) || defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__OpenBSD__) */
