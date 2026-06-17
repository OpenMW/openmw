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
    namespace Impl
    {
        inline std::filesystem::path searchRegistryPath(std::ifstream& stream,
            const std::filesystem::path& dosdevicesPath, std::string_view subKey, std::string_view valueName)
        {
            stream.seekg(0);

            bool isRegEntry = false;
            std::string line;
            while (std::getline(stream, line))
            {
                if (line.empty())
                    continue;
                if (line[0] == '[') // we found an entry
                {
                    if (isRegEntry)
                    {
                        break;
                    }

                    isRegEntry = line.find(subKey) != std::string::npos;
                }
                else if (isRegEntry && Misc::StringUtils::ciStartsWith(line, valueName))
                {
                    std::string path = line.substr(valueName.size(), line.rfind('"') - valueName.size());
                    if (path.empty())
                        break;
                    // Unescape path
                    for (auto it = path.begin(); it != path.end();)
                    {
                        if (*it != '\\')
                        {
                            ++it;
                            continue;
                        }
                        it = path.erase(it);
                        if (it == path.end())
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
                    path[0] = Misc::StringUtils::toLower(path[0]);
                    std::filesystem::path installPath = dosdevicesPath / path;

                    if (std::filesystem::is_directory(installPath))
                        return installPath;
                }
            }
            return {};
        }
    }

    inline std::vector<std::filesystem::path> getInstallPaths(const std::filesystem::path& homePath)
    {
        std::vector<std::filesystem::path> paths;

        const std::filesystem::path registryPath = homePath / ".wine/system.reg";
        if (!std::filesystem::is_regular_file(registryPath))
            return paths;

        std::ifstream registryFile(registryPath);
        if (!registryFile.is_open())
            return {};

        const std::filesystem::path dosdevicesPath = homePath / ".wine/dosdevices";
        {
            std::filesystem::path path = Impl::searchRegistryPath(
                registryFile, dosdevicesPath, "Bethesda Softworks\\\\Morrowind]", "\"Installed Path\"=\"");
            if (!path.empty())
                paths.emplace_back(std::move(path));
        }
        {
            std::filesystem::path path = Impl::searchRegistryPath(
                registryFile, dosdevicesPath, "GOG.com\\\\Games\\\\1435828767]", "\"PATH\"=\"");
            if (!path.empty())
                paths.emplace_back(std::move(path));
        }

        return paths;
    }
}

#endif /* COMPONENTS_FILES_WINEUTILS_HPP */
