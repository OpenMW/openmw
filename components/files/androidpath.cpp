#include "androidpath.hpp"

#if defined(__ANDROID__) 

#include <cstdlib>
#include <cstring>
#include <pwd.h>
#include <unistd.h>
#include <boost/filesystem/fstream.hpp>

namespace
{
    boost::filesystem::path getUserHome()
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

AndroidPath::AndroidPath(const std::string& application_name)
    : mName(application_name)
{
}

boost::filesystem::path AndroidPath::getUserConfigPath() const
{
    return getEnv("XDG_CONFIG_HOME",  "/sdcard/morrowind/config") / mName;
}

boost::filesystem::path AndroidPath::getUserDataPath() const
{
    return getEnv("XDG_DATA_HOME", "/sdcard/morrowind/share") / mName;
}

boost::filesystem::path AndroidPath::getCachePath() const
{
    return getEnv("XDG_CACHE_HOME", "/sdcard/morrowind/cache") / mName;
}

boost::filesystem::path AndroidPath::getGlobalConfigPath() const
{
    boost::filesystem::path globalPath("/sdcard/morrowind/"); 
    return globalPath / mName;
}

boost::filesystem::path AndroidPath::getLocalPath() const
{
    return boost::filesystem::path("./");
}

boost::filesystem::path AndroidPath::getGlobalDataPath() const
{
    boost::filesystem::path globalDataPath("/sdcard/morrowind/data");
    return globalDataPath / mName;
}


} /* namespace Files */

#endif /* defined(__Android__) */
