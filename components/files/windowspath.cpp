#include "windowspath.hpp"

#if defined(_WIN32) || defined(__WINDOWS__)

#include <cstring>

#include <windows.h>
#include <shlobj.h>
#include <Shlwapi.h>

#pragma comment(lib, "Shlwapi.lib")

/**
 * FIXME: Someone with Windows system should check this and correct if necessary
 */

/**
 * \namespace Files
 */
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

boost::filesystem::path WindowsPath::getGlobalDataPath() const
{
    return getGlobalPath();
}

boost::filesystem::path WindowsPath::getInstallPath() const
{
    boost::filesystem::path installPath("");

    HKEY hKey;

    BOOL f64 = FALSE;
    LPCTSTR regkey;
    if ((IsWow64Process(GetCurrentProcess(), &f64) && f64) || sizeof(void*) == 8)
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
        std::vector<char> buf(512);
        int len = 512;

        if (RegQueryValueEx(hKey, TEXT("Installed Path"), NULL, NULL, (LPBYTE)&buf[0], (LPDWORD)&len) == ERROR_SUCCESS)
        {
            installPath = &buf[0];
        }
    }

    return installPath;
}

} /* namespace Files */

#endif /* defined(_WIN32) || defined(__WINDOWS__) */
