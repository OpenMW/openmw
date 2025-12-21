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
                std::wstring buffer;
                buffer.reserve(MAX_PATH);
                DWORD len = static_cast<DWORD>(MAX_PATH * sizeof(wchar_t));

                auto result = RegQueryValueExW(
                    key.mKey, valueName, nullptr, nullptr, reinterpret_cast<LPBYTE>(buffer.data()), &len);
                if (result == ERROR_MORE_DATA)
                {
                    buffer.reserve(len / sizeof(wchar_t));
                    result = RegQueryValueExW(
                        key.mKey, valueName, nullptr, nullptr, reinterpret_cast<LPBYTE>(buffer.data()), &len);
                }
                if (result == ERROR_SUCCESS)
                {
                    // This should always be true. Note that we don't need to care above because of the trailing \0
                    if (len % sizeof(wchar_t) == 0)
                        return std::filesystem::path(buffer.data(), buffer.data() + len / sizeof(wchar_t));
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

        std::wstring executablePath;
        DWORD copied = 0;
        do
        {
            executablePath.resize(executablePath.size() + MAX_PATH);
            copied = GetModuleFileNameW(nullptr, executablePath.data(), static_cast<DWORD>(executablePath.size()));
        } while (GetLastError() == ERROR_INSUFFICIENT_BUFFER);

        if (copied > 0)
        {
            localPath = std::filesystem::path(executablePath).parent_path() / "";
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
