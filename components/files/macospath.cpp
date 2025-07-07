#include "macospath.hpp"

#if defined(macintosh) || defined(Macintosh) || defined(__APPLE__) || defined(__MACH__)

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <mach-o/dyld.h>
#include <pwd.h>
#include <unistd.h>
#include <vector>

#include <components/debug/debuglog.hpp>
#include <components/misc/strings/lower.hpp>

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

    std::optional<std::filesystem::path> MacOsPath::getGlobalConfigPath() const
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

    std::optional<std::filesystem::path> MacOsPath::getGlobalDataPath() const
    {
        std::filesystem::path globalDataPath("/Library/Application Support/");
        return globalDataPath / mName;
    }

    std::filesystem::path MacOsPath::getInstallPath() const
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
                    // Change drive letter to lowercase, so we could use ~/.wine/dosdevice symlinks
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

#endif /* defined(macintosh) || defined(Macintosh) || defined(__APPLE__) || defined(__MACH__) */
