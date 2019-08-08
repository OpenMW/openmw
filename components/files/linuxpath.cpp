#include "linuxpath.hpp"

#if defined(__linux__) || defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__OpenBSD__)

#include <pwd.h>
#include <unistd.h>
#include <boost/filesystem/fstream.hpp>

#include <components/debug/debuglog.hpp>
#include <components/misc/stringops.hpp>


namespace
{
    boost::filesystem::path getUserHome()
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
            return boost::filesystem::path();
        else
            return boost::filesystem::path(dir);
    }

    boost::filesystem::path getEnv(const std::string& envVariable, const boost::filesystem::path& fallback)
    {
        const char* result = getenv(envVariable.c_str());
        if (!result)
            return fallback;
        boost::filesystem::path dir(result);
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
    boost::filesystem::path localPath = getLocalPath();
    if (chdir(localPath.string().c_str()) != 0)
        Log(Debug::Warning) << "Error " << errno << " when changing current directory";
}

boost::filesystem::path LinuxPath::getUserConfigPath() const
{
    return getEnv("XDG_CONFIG_HOME", getUserHome() / ".config") / mName;
}

boost::filesystem::path LinuxPath::getUserDataPath() const
{
    return getEnv("XDG_DATA_HOME", getUserHome() / ".local/share") / mName;
}

boost::filesystem::path LinuxPath::getCachePath() const
{
    return getEnv("XDG_CACHE_HOME", getUserHome() / ".cache") / mName;
}

boost::filesystem::path LinuxPath::getGlobalConfigPath() const
{
    boost::filesystem::path globalPath(GLOBAL_CONFIG_PATH);
    return globalPath / mName;
}

boost::filesystem::path LinuxPath::getLocalPath() const
{
    boost::filesystem::path localPath("./");
    std::string binPath(pathconf(".", _PC_PATH_MAX), '\0');
    const char *statusPaths[] = {"/proc/self/exe", "/proc/self/file", "/proc/curproc/exe", "/proc/curproc/file"};

    for(const char *path : statusPaths)
    {
        if (readlink(path, &binPath[0], binPath.size()) != -1)
        {
            localPath = boost::filesystem::path(binPath).parent_path() / "/";
            break;
        }
    }

    return localPath;
}

boost::filesystem::path LinuxPath::getGlobalDataPath() const
{
    boost::filesystem::path globalDataPath(GLOBAL_DATA_PATH);
    return globalDataPath / mName;
}

boost::filesystem::path LinuxPath::getInstallPath() const
{
    boost::filesystem::path installPath;

    boost::filesystem::path homePath = getUserHome();

    if (!homePath.empty())
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
                // Change drive letter to lowercase, so we could use
                // ~/.wine/dosdevices symlinks
                mwpath[0] = Misc::StringUtils::toLower(mwpath[0]);
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

#endif /* defined(__linux__) || defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__OpenBSD__) */
