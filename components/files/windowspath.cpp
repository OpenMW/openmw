#include "windowspath.hpp"

#if defined(_WIN32) || defined(__WINDOWS__)

#include <cstring>

#define FAR
#define NEAR

#include <shlobj.h>
#include <shlwapi.h>
#include <winreg.h>

#undef NEAR
#undef FAR

#include <boost/locale.hpp>
namespace bconv = boost::locale::conv;

#include <components/debug/debuglog.hpp>

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

        See std::filesystem and boost::locale reference for details.
    */
    // TODO(Project579): Temporarly disabled until a good solution is found (no solution might actually be needed)
    //std::filesystem::path::imbue(boost::locale::generator().generate(""));

    std::filesystem::path localPath = getLocalPath();
    if (!SetCurrentDirectoryA(localPath.string().c_str()))
        Log(Debug::Warning) << "Error " << GetLastError() << " when changing current directory";
}

std::filesystem::path WindowsPath::getUserConfigPath() const
{
    std::filesystem::path userPath(".");

    WCHAR path[MAX_PATH + 1];
    memset(path, 0, sizeof(path));

    if(SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_PERSONAL | CSIDL_FLAG_CREATE, nullptr, 0, path)))
    {
        userPath = std::filesystem::path(bconv::utf_to_utf<char>(path));
    }

    return userPath / "My Games" / mName;
}

std::filesystem::path WindowsPath::getUserDataPath() const
{
    // Have some chaos, windows people!
    return getUserConfigPath();
}

std::filesystem::path WindowsPath::getGlobalConfigPath() const
{
    std::filesystem::path globalPath(".");

    WCHAR path[MAX_PATH + 1];
    memset(path, 0, sizeof(path));

    if(SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_PROGRAM_FILES | CSIDL_FLAG_CREATE, nullptr, 0, path)))
    {
        globalPath = std::filesystem::path(bconv::utf_to_utf<char>(path));
    }

    return globalPath / mName;
}

std::filesystem::path WindowsPath::getLocalPath() const
{
    std::filesystem::path localPath("./");
    WCHAR path[MAX_PATH + 1];
    memset(path, 0, sizeof(path));

    if (GetModuleFileNameW(nullptr, path, MAX_PATH + 1) > 0)
    {
        localPath = std::filesystem::path(bconv::utf_to_utf<char>(path)).parent_path().string() + "/";
    }

    // lookup exe path
    return localPath;
}

std::filesystem::path WindowsPath::getGlobalDataPath() const
{
    return getGlobalConfigPath();
}

std::filesystem::path WindowsPath::getCachePath() const
{
    return getUserConfigPath() / "cache";
}

std::filesystem::path WindowsPath::getInstallPath() const
{
    std::filesystem::path installPath("");

    HKEY hKey;

    LPCTSTR regkey = TEXT("SOFTWARE\\Bethesda Softworks\\Morrowind");
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, regkey, 0, KEY_READ | KEY_WOW64_32KEY, &hKey) == ERROR_SUCCESS)
    {
        //Key existed, let's try to read the install dir
        std::vector<char> buf(512);
        int len = 512;

        if (RegQueryValueEx(hKey, TEXT("Installed Path"), nullptr, nullptr, (LPBYTE)buf.data(), (LPDWORD)&len) == ERROR_SUCCESS)
        {
            installPath = buf.data();
        }
        RegCloseKey(hKey);
    }

    return installPath;
}

} /* namespace Files */

#endif /* defined(_WIN32) || defined(__WINDOWS__) */
