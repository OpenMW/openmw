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

        std::filesystem::path getRegistryPath(LPCWSTR subKey, LPCWSTR valueName, bool use32)
        {
            RegistryKey key;
            REGSAM flags = KEY_READ;
            if (use32)
                flags |= KEY_WOW64_32KEY;
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, subKey, 0, flags, &key.mKey) == ERROR_SUCCESS)
            {
                // Key existed, let's try to read the install dir
                std::array<wchar_t, 512> buf{};
                DWORD len = static_cast<DWORD>(buf.size() * sizeof(wchar_t));

                if (RegQueryValueExW(key.mKey, valueName, nullptr, nullptr, reinterpret_cast<LPBYTE>(buf.data()), &len)
                    == ERROR_SUCCESS)
                {
                    // This should always be true
                    if (len % sizeof(wchar_t) == 0)
                        return std::filesystem::path(buf.data(), buf.data() + len / sizeof(wchar_t));
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

    std::vector<std::filesystem::path> WindowsPath::getInstallPaths() const
    {
        std::vector<std::filesystem::path> paths;
        {
            std::filesystem::path disk
                = getRegistryPath(L"SOFTWARE\\Bethesda Softworks\\Morrowind", L"Installed Path", true);
            if (!disk.empty() && std::filesystem::is_directory(disk))
                paths.emplace_back(std::move(disk));
        }
        {
            std::filesystem::path steam = getRegistryPath(
                L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam App 22320", L"InstallLocation", false);
            if (!steam.empty() && std::filesystem::is_directory(steam))
                paths.emplace_back(std::move(steam));
        }
        return paths;
    }

} /* namespace Files */

#endif /* defined(_WIN32) || defined(__WINDOWS__) */
