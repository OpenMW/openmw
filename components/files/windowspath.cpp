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
    namespace
    {
        struct RegistryKey
        {
            HKEY mKey = nullptr;

            ~RegistryKey()
            {
                if (mKey)
                    RegCloseKey(mKey);
            }
        };

        std::filesystem::path getRegistryPath(LPCWSTR subKey, LPCWSTR valueName)
        {
            RegistryKey key;
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, subKey, 0, KEY_READ | KEY_WOW64_32KEY, &key.mKey) == ERROR_SUCCESS)
            {
                // Key existed, let's try to read the install dir
                std::array<wchar_t, 512> buf{};
                DWORD len = static_cast<DWORD>(buf.size() * sizeof(wchar_t));

                if (RegQueryValueExW(key.mKey, valueName, nullptr, nullptr, reinterpret_cast<LPBYTE>(buf.data()), &len)
                    == ERROR_SUCCESS)
                {
                    return std::filesystem::path(buf.data(), buf.data() + len);
                }
            }
            return {};
        }
    }

    WindowsPath::WindowsPath(const std::string& application_name)
        : mName(application_name)
    {
    }

    std::filesystem::path WindowsPath::getUserConfigPath() const
    {
        std::filesystem::path userPath = std::filesystem::current_path();

        PWSTR cString;
        HRESULT result = SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &cString);
        if (SUCCEEDED(result))
            userPath = std::filesystem::path(cString);
        else
            Log(Debug::Error) << "Error " << result << " when getting Documents path";

        CoTaskMemFree(cString);

        return userPath / "My Games" / mName;
    }

    std::filesystem::path WindowsPath::getUserDataPath() const
    {
        // Have some chaos, windows people!
        return getUserConfigPath();
    }

    std::filesystem::path WindowsPath::getGlobalConfigPath() const
    {
        // The concept of a global config path is absurd on Windows.
        // Always use local config instead.
        return {};
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
        std::filesystem::path installPath
            = getRegistryPath(L"SOFTWARE\\Bethesda Softworks\\Morrowind", L"Installed Path");
        if (!installPath.empty() && std::filesystem::is_directory(installPath))
            return installPath;
        return {};
    }

} /* namespace Files */

#endif /* defined(_WIN32) || defined(__WINDOWS__) */
