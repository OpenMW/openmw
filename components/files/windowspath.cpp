#include "windowspath.hpp"

#if defined(_WIN32) || defined(__WINDOWS__)

#include <array>
#include <cstring>

#define FAR
#define NEAR

#include <shlobj.h>
#include <shlwapi.h>
#include <winreg.h>

#undef NEAR
#undef FAR

#include <components/debug/debuglog.hpp>

/**
 * \namespace Files
 */
namespace Files
{

    WindowsPath::WindowsPath(const std::string& application_name)
        : mName(application_name)
    {
        std::error_code ec;
        current_path(getLocalPath(), ec);
        if (ec.value() != 0)
            Log(Debug::Warning) << "Error " << ec.value() << " when changing current directory";
    }

    std::filesystem::path WindowsPath::getUserConfigPath() const
    {
        std::filesystem::path userPath = std::filesystem::current_path();

        WCHAR path[MAX_PATH + 1] = {};

        if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_PERSONAL | CSIDL_FLAG_CREATE, nullptr, 0, path)))
        {
            userPath = std::filesystem::path(path);
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
        std::filesystem::path globalPath = std::filesystem::current_path();

        WCHAR path[MAX_PATH + 1] = {};

        if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_PROGRAM_FILES | CSIDL_FLAG_CREATE, nullptr, 0, path)))
        {
            globalPath = std::filesystem::path(path);
        }

        return globalPath / mName;
    }

    std::filesystem::path WindowsPath::getLocalPath() const
    {
        std::filesystem::path localPath = std::filesystem::current_path() / "";

        WCHAR path[MAX_PATH + 1] = {};

        if (GetModuleFileNameW(nullptr, path, MAX_PATH + 1) > 0)
        {
            localPath = std::filesystem::path(path).parent_path() / "";
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
        std::filesystem::path installPath{};

        if (HKEY hKey; RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Bethesda Softworks\\Morrowind", 0,
                           KEY_READ | KEY_WOW64_32KEY, &hKey)
            == ERROR_SUCCESS)
        {
            // Key existed, let's try to read the install dir
            std::array<wchar_t, 512> buf{};
            DWORD len = static_cast<DWORD>(buf.size() * sizeof(wchar_t));

            if (RegQueryValueExW(hKey, L"Installed Path", nullptr, nullptr, reinterpret_cast<LPBYTE>(buf.data()), &len)
                == ERROR_SUCCESS)
            {
                installPath = std::filesystem::path(buf.data());
            }
            RegCloseKey(hKey);
        }

        return installPath;
    }

} /* namespace Files */

#endif /* defined(_WIN32) || defined(__WINDOWS__) */
