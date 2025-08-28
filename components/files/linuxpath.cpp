#include "linuxpath.hpp"

#if defined(__linux__) || defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__OpenBSD__)

#include <cstring>
#include <fstream>
#include <pwd.h>
#include <unistd.h>

#include <components/misc/strings/lower.hpp>

namespace
{
    std::filesystem::path getUserHome()
    {
        const char* dir = getenv("HOME");
        if (dir == nullptr)
        {
            struct passwd* pwd = getpwuid(getuid());
            if (pwd != nullptr)
            {
                dir = pwd->pw_dir;
            }
        }
        if (dir == nullptr)
            return {};
        else
            return dir;
    }

    std::filesystem::path getEnv(const std::string& envVariable, const std::filesystem::path& fallback)
    {
        const char* result = getenv(envVariable.c_str());
        if (!result)
            return fallback;
        std::filesystem::path dir(result);
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

    LinuxPath::LinuxPath(const std::string& applicationName)
        : mName(applicationName)
    {
    }

    std::filesystem::path LinuxPath::getUserConfigPath() const
    {
        return getEnv("XDG_CONFIG_HOME", getUserHome() / ".config") / mName;
    }

    std::filesystem::path LinuxPath::getUserDataPath() const
    {
        return getEnv("XDG_DATA_HOME", getUserHome() / ".local/share") / mName;
    }

    std::filesystem::path LinuxPath::getCachePath() const
    {
        return getEnv("XDG_CACHE_HOME", getUserHome() / ".cache") / mName;
    }

    std::filesystem::path LinuxPath::getGlobalConfigPath() const
    {
        std::filesystem::path globalPath(GLOBAL_CONFIG_PATH);
        return globalPath / mName;
    }

    std::filesystem::path LinuxPath::getLocalPath() const
    {
        auto localPath = std::filesystem::current_path() / "";

        static const std::filesystem::path statusPaths[]
            = { "/proc/self/exe", "/proc/self/file", "/proc/curproc/exe", "/proc/curproc/file" };

        for (const auto& path : statusPaths)
        {
            std::error_code ec;
            const auto binPath = read_symlink(path, ec);
            if (ec.value() != -1)
            {
                localPath = binPath.parent_path() / "";
                break;
            }
        }

        return localPath;
    }

    std::filesystem::path LinuxPath::getGlobalDataPath() const
    {
        std::filesystem::path globalDataPath(GLOBAL_DATA_PATH);
        return globalDataPath / mName;
    }

    std::filesystem::path LinuxPath::getInstallPath() const
    {
        std::filesystem::path installPath;

        std::filesystem::path homePath = getUserHome();

        if (!homePath.empty())
        {
            std::filesystem::path wineDefaultRegistry(homePath);
            wineDefaultRegistry /= ".wine/system.reg";

            if (std::filesystem::is_regular_file(wineDefaultRegistry))
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

                    if (!std::filesystem::is_directory(installPath))
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
