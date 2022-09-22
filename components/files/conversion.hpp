#ifndef COMPONENTS_FILES_CONVERSION_HPP
#define COMPONENTS_FILES_CONVERSION_HPP

#include <filesystem>

namespace Files
{
    std::string pathToUnicodeString(const std::filesystem::path& path);

    std::string pathToUnicodeString(std::filesystem::path&& path);

    std::filesystem::path pathFromUnicodeString(std::string_view path);

    std::filesystem::path pathFromUnicodeString(std::string&& path);

    std::filesystem::path pathFromUnicodeString(const char* path);
}

#endif // COMPONENTS_FILES_CONFIGURATIONMANAGER_HPP
