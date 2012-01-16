#include "windowspath.hpp"

#if defined(_WIN32) || defined(__WINDOWS__)

#include <cstring>

#include <windows.h>
#include <shlobj.h>
#include <Shlwapi.h>

#pragma comment(lib, "Shlwapi.lib")

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

boost::filesystem::path WindowsPath::getInstallPath() const
{
    boost::filesystem::path installPath("");

    HKEY hKey;

    BOOL f64 = FALSE;
    LPCTSTR regkey;
    if (IsWow64Process(GetCurrentProcess(), &f64) && f64)
    {
        regkey = "SOFTWARE\\Wow6432Node\\Bethesda Softworks\\Morrowind";
    }
    else
    {
        regkey = "SOFTWARE\\Bethesda Softworks\\Morrowind";
    }

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT(regkey), 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
    {
        //Key existed, let's try to read the install dir
        char* data = new char[4096];
        int len = 4096;

        if (RegQueryValueEx(hKey, TEXT("Installed Path"), NULL, NULL, (LPBYTE)data, (LPDWORD)&len) == ERROR_SUCCESS)
        {
            installPath = data;
        }

        delete[] data;
    }

    return installPath;
}

} /* namespace Files */

#endif /* defined(_WIN32) || defined(__WINDOWS__) */
