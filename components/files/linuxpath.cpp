#include "linuxpath.hpp"

#if defined(__linux__) || defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__OpenBSD__)

#include <pwd.h>
#include <unistd.h>
#include <fstream>
#include <string.h>

#include <components/misc/stringops.hpp>


namespace
{
    std::experimental::filesystem::path getUserHome()
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
            return std::experimental::filesystem::path();
        else
            return std::experimental::filesystem::path(dir);
    }

    std::experimental::filesystem::path getEnv(const std::string& envVariable, const std::experimental::filesystem::path& fallback)
    {
        const char* result = getenv(envVariable.c_str());
        if (!result)
            return fallback;
        std::experimental::filesystem::path dir(result);
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

std::experimental::filesystem::path LinuxPath::getUserConfigPath() const
{
    return getEnv("XDG_CONFIG_HOME", getUserHome() / ".config") / mName;
}

std::experimental::filesystem::path LinuxPath::getUserDataPath() const
{
    return getEnv("XDG_DATA_HOME", getUserHome() / ".local/share") / mName;
}

std::experimental::filesystem::path LinuxPath::getCachePath() const
{
    return getEnv("XDG_CACHE_HOME", getUserHome() / ".cache") / mName;
}

std::experimental::filesystem::path LinuxPath::getGlobalConfigPath() const
{
    std::experimental::filesystem::path globalPath(GLOBAL_CONFIG_PATH);
    return globalPath / mName;
}

std::experimental::filesystem::path LinuxPath::getLocalPath() const
{
    return std::experimental::filesystem::path("./");
}

std::experimental::filesystem::path LinuxPath::getGlobalDataPath() const
{
    std::experimental::filesystem::path globalDataPath(GLOBAL_DATA_PATH);
    return globalDataPath / mName;
}

std::experimental::filesystem::path LinuxPath::getInstallPath() const
{
    std::experimental::filesystem::path installPath;

    std::experimental::filesystem::path homePath = getUserHome();

    if (!homePath.empty())
    {
        std::experimental::filesystem::path wineDefaultRegistry(homePath);
        wineDefaultRegistry /= ".wine/system.reg";

        if (std::experimental::filesystem::is_regular_file(wineDefaultRegistry))
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

                if (!std::experimental::filesystem::is_directory(installPath))
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
