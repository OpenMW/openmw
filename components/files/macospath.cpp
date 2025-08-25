#include "macospath.hpp"

#if defined(macintosh) || defined(Macintosh) || defined(__APPLE__) || defined(__MACH__)

#include <cstdlib>
#include <filesystem>
#include <mach-o/dyld.h>
#include <pwd.h>
#include <unistd.h>
#include <vector>

#include <components/debug/debuglog.hpp>

#include "wine.hpp"

namespace
{
    std::filesystem::path getBinaryPath()
    {
        uint32_t bufsize = 0;
        _NSGetExecutablePath(nullptr, &bufsize);

        std::vector<char> buf(bufsize);

        if (_NSGetExecutablePath(buf.data(), &bufsize) == 0)
        {
            std::filesystem::path path = std::filesystem::path(buf.begin(), buf.end());

            if (std::filesystem::is_symlink(path))
            {
                return std::filesystem::read_symlink(path);
            }

            return path;
        }
        else
        {
            Log(Debug::Warning) << "Not enough buffer size to get executable path: " << bufsize;
            throw std::runtime_error("Failed to get executable path");
        }
    }

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
            return std::filesystem::path();
        else
            return std::filesystem::path(dir);
    }
}

namespace Files
{

    MacOsPath::MacOsPath(const std::string& application_name)
        : mName(application_name)
    {
    }

    std::filesystem::path MacOsPath::getUserConfigPath() const
    {
        std::filesystem::path userPath(getUserHome());
        userPath /= "Library/Preferences/";

        return userPath / mName;
    }

    std::filesystem::path MacOsPath::getUserDataPath() const
    {
        std::filesystem::path userPath(getUserHome());
        userPath /= "Library/Application Support/";

        return userPath / mName;
    }

    std::filesystem::path MacOsPath::getGlobalConfigPath() const
    {
        std::filesystem::path globalPath("/Library/Preferences/");
        return globalPath / mName;
    }

    std::filesystem::path MacOsPath::getCachePath() const
    {
        std::filesystem::path userPath(getUserHome());
        userPath /= "Library/Caches";
        return userPath / mName;
    }

    std::filesystem::path MacOsPath::getLocalPath() const
    {
        return getBinaryPath().parent_path().parent_path() / "Resources";
    }

    std::filesystem::path MacOsPath::getGlobalDataPath() const
    {
        std::filesystem::path globalDataPath("/Library/Application Support/");
        return globalDataPath / mName;
    }

    std::vector<std::filesystem::path> MacOsPath::getInstallPaths() const
    {
        std::vector<std::filesystem::path> paths;
        std::filesystem::path homePath = getUserHome();
        if (!homePath.empty())
        {
            std::filesystem::path wine = Wine::getInstallPath(homePath);
            if (!wine.empty())
                paths.emplace_back(std::move(wine));
        }
        return paths;
    }

} /* namespace Files */

#endif /* defined(macintosh) || defined(Macintosh) || defined(__APPLE__) || defined(__MACH__) */
