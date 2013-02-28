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

MacOsPath::MacOsPath(const std::string& application_name)
    : mName(application_name)
{
}

boost::filesystem::path MacOsPath::getUserPath() const
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
        userPath = boost::filesystem::path(theDir) / "Library/Preferences/";
    }

    return userPath / mName;
}

boost::filesystem::path MacOsPath::getGlobalPath() const
{
    boost::filesystem::path globalPath("/Library/Preferences/");
    return globalPath / mName;
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
        userPath = boost::filesystem::path(theDir) / "Library/Caches" / mName;
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
    return globalDataPath / mName;
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
