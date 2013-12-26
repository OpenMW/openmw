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

WindowsPath::WindowsPath(const std::string& application_name)
    : mName(application_name)
{
}

boost::filesystem::path WindowsPath::getUserConfigPath() const
{
    boost::filesystem::path userPath(".");

    TCHAR path[MAX_PATH];
    memset(path, 0, sizeof(path));

    if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PERSONAL | CSIDL_FLAG_CREATE, NULL, 0, path)))
    {
        PathAppend(path, TEXT("My Games"));
        userPath = boost::filesystem::path(path);
    }

    return userPath / mName;
}

boost::filesystem::path WindowsPath::getUserDataPath() const
{
    // Have some chaos, windows people!
    return getUserConfigPath();
}

boost::filesystem::path WindowsPath::getGlobalConfigPath() const
{
    boost::filesystem::path globalPath(".");

    TCHAR path[MAX_PATH];
    memset(path, 0, sizeof(path));

    if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PROGRAM_FILES | CSIDL_FLAG_CREATE, NULL, 0, path)))
    {
        globalPath = boost::filesystem::path(path);
    }

    return globalPath / mName;
}

boost::filesystem::path WindowsPath::getLocalPath() const
{
    return boost::filesystem::path("./");
}

boost::filesystem::path WindowsPath::getGlobalDataPath() const
{
    return getGlobalPath();
}

boost::filesystem::path WindowsPath::getCachePath() const
{
    return getUserPath() / "cache";
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
