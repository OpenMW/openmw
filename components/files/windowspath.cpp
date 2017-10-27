#include "windowspath.hpp"

#if defined(_WIN32) || defined(__WINDOWS__)

#include <cstring>

#include <shlobj.h>
#include <shlwapi.h>
#include <WinReg.h>

#include <boost/locale.hpp>
namespace bconv = boost::locale::conv;

/**
 * FIXME: Someone with Windows system should check this and correct if necessary
 * FIXME: MAX_PATH is irrelevant for extended-length paths, i.e. \\?\...
 */

/**
 * \namespace Files
 */
namespace Files
{

WindowsPath::WindowsPath(const std::string& application_name)
    : mName(application_name)
{
    /*  Since on Windows boost::path.string() returns string of narrow
        characters in local encoding, it is required to path::imbue()
        with UTF-8 encoding (generated for empty name from boost::locale)
        to handle Unicode in platform-agnostic way using std::string.

        See boost::filesystem and boost::locale reference for details.
    */
    sfs::path::imbue(boost::locale::generator().generate(""));
}

sfs::path WindowsPath::getUserConfigPath() const
{
    sfs::path userPath(".");

    WCHAR path[MAX_PATH + 1];
    memset(path, 0, sizeof(path));

    if(SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PERSONAL | CSIDL_FLAG_CREATE, NULL, 0, path)))
    {
        userPath = sfs::path(bconv::utf_to_utf<char>(path));
    }

    return userPath / "My Games" / mName;
}

sfs::path WindowsPath::getUserDataPath() const
{
    // Have some chaos, windows people!
    return getUserConfigPath();
}

sfs::path WindowsPath::getGlobalConfigPath() const
{
    sfs::path globalPath(".");

    WCHAR path[MAX_PATH + 1];
    memset(path, 0, sizeof(path));

    if(SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PROGRAM_FILES | CSIDL_FLAG_CREATE, NULL, 0, path)))
    {
        globalPath = sfs::path(bconv::utf_to_utf<char>(path));
    }

    return globalPath / mName;
}

sfs::path WindowsPath::getLocalPath() const
{
    return sfs::path("./");
}

sfs::path WindowsPath::getGlobalDataPath() const
{
    return getGlobalConfigPath();
}

sfs::path WindowsPath::getCachePath() const
{
    return getUserConfigPath() / "cache";
}

sfs::path WindowsPath::getInstallPath() const
{
    sfs::path installPath("");

    HKEY hKey;

    LPCTSTR regkey = TEXT("SOFTWARE\\Bethesda Softworks\\Morrowind");
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, regkey, 0, KEY_READ | KEY_WOW64_32KEY, &hKey) == ERROR_SUCCESS)
    {
        //Key existed, let's try to read the install dir
        std::vector<char> buf(512);
        int len = 512;

        if (RegQueryValueEx(hKey, TEXT("Installed Path"), NULL, NULL, (LPBYTE)&buf[0], (LPDWORD)&len) == ERROR_SUCCESS)
        {
            installPath = &buf[0];
        }
        RegCloseKey(hKey);
    }

    return installPath;
}

} /* namespace Files */

#endif /* defined(_WIN32) || defined(__WINDOWS__) */
