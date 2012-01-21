#include "windowspath.hpp"

#if defined(_WIN32) || defined(__WINDOWS__)

#include <cstring>

#include <windows.h>
#include <shobj.h>

namespace Files
{

boost::filesystem::path WindowsPath::getUserPath() const
{
    boost::filesystem::path userPath(".");
    boost::filesystem::path suffix("/");

    TCHAR path[MAX_PATH];
    memset(path, 0, sizeof(path));

    if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PERSONAL | CSIDL_FLAG_CREATE, NULL, 0, path)))
    {
        PathAppend(path, TEXT("My Games"));
        userPath = boost::filesystem::path(path);
    }

    userPath /= suffix;

    return userPath;
}

boost::filesystem::path WindowsPath::getGlobalPath() const
{
    boost::filesystem::path globalPath(".");
    boost::filesystem::path suffix("/");

    TCHAR path[MAX_PATH];
    memset(path, 0, sizeof(path));

    if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PROGRAM_FILES | CSIDL_FLAG_CREATE, NULL, 0, path)))
    {
        globalPath = boost::filesystem::path(path);
    }

    globalPath /= suffix;

    return globalPath;
}

boost::filesystem::path WindowsPath::getLocalPath() const
{
    return boost::filesystem::path("./");
}

/**
 * FIXME: Someone with Windows system should check this and correct if necessary
 */
boost::filesystem::path WindowsPath::getUserDataPath() const
{
    return getUserConfigPath();
}

/**
 * FIXME: Someone with Windows system should check this and correct if necessary
 */
boost::filesystem::path WindowsPath::getGlobalDataPath() const
{
    return getGlobalConfigPath();
}

/**
 * FIXME: Someone with Windows system should check this and correct if necessary
 */
boost::filesystem::path WindowsPath::getLocalDataPath() const
{
    return boost::filesystem::path("./data/");
}

boost::filesystem::path WindowsPath::getInstallPath() const;
{
    return boost::filesystem::path("./");
}

} /* namespace Files */

#endif /* defined(_WIN32) || defined(__WINDOWS__) */
