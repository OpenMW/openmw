#include "windowspath.hpp"

#if defined(_WIN32) || defined(__WINDOWS__)

#include <cstring>

#include <windows.h>
#include <shobj.h>

namespace Files
{

boost::filesystem::path WindowsPath::getLocalConfigPath() const
{
    boost::filesystem::path localConfigPath(".");
    boost::filesystem::path suffix("/");

    TCHAR path[MAX_PATH];
    memset(path, 0, sizeof(path));

    if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PERSONAL | CSIDL_FLAG_CREATE, NULL, 0, path)))
    {
        PathAppend(path, TEXT("My Games"));
        localConfigPath = boost::filesystem::path(path);
    }

    localConfigPath /= suffix;

    return localConfigPath;
}

boost::filesystem::path WindowsPath::getGlobalConfigPath() const
{
    boost::filesystem::path globalConfigPath(".");
    boost::filesystem::path suffix("/");

    TCHAR path[MAX_PATH];
    memset(path, 0, sizeof(path));

    if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PROGRAM_FILES | CSIDL_FLAG_CREATE, NULL, 0, path)))
    {
        globalConfigPath = boost::filesystem::path(path);
    }

    globalConfigPath /= suffix;

    return globalConfigPath;
}

boost::filesystem::path WindowsPath::getRuntimeConfigPath() const
{
    return boost::filesystem::path("./");
}

boost::filesystem::path WindowsPath::getLocalDataPath() const
{
    return getLocalConfigPath();
}

boost::filesystem::path WindowsPath::getGlobalDataPath() const
{
    return getGlobalConfigPath();
}

boost::filesystem::path WindowsPath::getRuntimeDataPath() const
{
    return boost::filesystem::path("./data/");
}

} /* namespace Files */

#endif /* defined(_WIN32) || defined(__WINDOWS__) */
