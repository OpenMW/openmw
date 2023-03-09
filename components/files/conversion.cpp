
#include "conversion.hpp"

#include <components/misc/strings/conversion.hpp>

std::string Files::pathToUnicodeString(const std::filesystem::path& path)
{
    return path.u8string();//Misc::StringUtils::u8StringToString(path.u8string());
}

std::string Files::pathToUnicodeString(std::filesystem::path&& path)
{
    return path.u8string();//Misc::StringUtils::u8StringToString(path.u8string());
}

std::filesystem::path Files::pathFromUnicodeString(std::string_view path)
{
    return path;//Misc::StringUtils::stringToU8String(path);
}

std::filesystem::path Files::pathFromUnicodeString(std::string&& path)
{
    return std::move(path);//Misc::StringUtils::stringToU8String(std::move(path));
}

std::filesystem::path Files::pathFromUnicodeString(const char* path)
{
    return path;//Misc::StringUtils::stringToU8String(path);
}
