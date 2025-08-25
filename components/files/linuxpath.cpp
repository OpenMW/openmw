#include "linuxpath.hpp"

#if defined(__linux__) || defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__OpenBSD__)

#include <array>
#include <cstring>
#include <pwd.h>
#include <unistd.h>

#include <components/misc/strings/lower.hpp>

#include "wine.hpp"

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

    std::vector<std::filesystem::path> LinuxPath::getInstallPaths() const
    {
        std::vector<std::filesystem::path> paths;
        std::filesystem::path homePath = getUserHome();
        if (!homePath.empty())
        {
            std::filesystem::path wine = Wine::getInstallPath(homePath);
            if (!wine.empty())
                paths.emplace_back(std::move(wine));
            constexpr std::string_view steamPath = ".local/share/Steam/steamapps/common/Morrowind";
            std::array steamPaths{
                homePath / steamPath, // Default
                homePath / "snap/steam/common" / steamPath, // Snap
                homePath / ".var/app/com.valvesoftware.Steam" / steamPath, // Flatpak
            };
            for (std::filesystem::path steam : steamPaths)
            {
                if (std::filesystem::is_directory(steam))
                    paths.emplace_back(std::move(steam));
            }
        }
        return paths;
    }

} /* namespace Files */

#endif /* defined(__linux__) || defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__OpenBSD__) */
