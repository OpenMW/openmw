#ifndef COMPONENTS_FILES_WINEUTILS_HPP
#define COMPONENTS_FILES_WINEUTILS_HPP

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

#include <components/misc/strings/algorithm.hpp>

namespace Files::Wine
{
    inline std::filesystem::path getInstallPath(const std::filesystem::path& homePath)
    {
        std::filesystem::path wineDefaultRegistry = homePath / ".wine/system.reg";
        if (!std::filesystem::is_regular_file(wineDefaultRegistry))
            return {};

        constexpr std::string_view keyStart = "\"Installed Path\"=\"";
        std::ifstream file(wineDefaultRegistry);
        bool isRegEntry = false;
        std::string line;
        while (std::getline(file, line))
        {
            if (line.empty())
                continue;
            if (line[0] == '[') // we found an entry
            {
                if (isRegEntry)
                {
                    break;
                }

                isRegEntry = line.find("Softworks\\\\Morrowind]") != std::string::npos;
            }
            else if (isRegEntry && Misc::StringUtils::ciStartsWith(line, keyStart))
            {
                std::string mwpath = line.substr(keyStart.size(), line.rfind('"') - keyStart.size());
                if (mwpath.empty())
                    break;
                // Unescape path
                for (auto it = mwpath.begin(); it != mwpath.end();)
                {
                    if (*it != '\\')
                    {
                        ++it;
                        continue;
                    }
                    it = mwpath.erase(it);
                    if (it == mwpath.end())
                        return {}; // Invalid string
                    char& c = *it;
                    // Replace \ with /
                    if (c == '\\')
                        c = '/';
                    else // And just give up on any non-\ value
                        return {};
                    ++it;
                }
                // Change drive letter to lowercase, so we could use ~/.wine/dosdevices symlinks
                mwpath[0] = Misc::StringUtils::toLower(mwpath[0]);
                std::filesystem::path installPath = homePath / ".wine/dosdevices/" / mwpath;

                if (std::filesystem::is_directory(installPath))
                    return installPath;
            }
        }
        return {};
    }
}

#endif /* COMPONENTS_FILES_WINEUTILS_HPP */
