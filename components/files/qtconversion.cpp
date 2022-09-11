
#include "qtconversion.hpp"

#include <components/misc/strings/conversion.hpp>

QString Files::pathToQString(const std::filesystem::path& path)
{
    const auto tmp = path.u8string();
    return QString::fromUtf8(Misc::StringUtils::u8StringToString(tmp.data()), tmp.size());
}

QString Files::pathToQString(std::filesystem::path&& path)
{
    const auto tmp = path.u8string();
    return QString::fromUtf8(Misc::StringUtils::u8StringToString(tmp.data()), tmp.size());
}

std::filesystem::path Files::pathFromQString(QStringView path)
{
    const auto tmp = path.toUtf8();
    return std::filesystem::path{ Misc::StringUtils::stringToU8String(tmp) };
}

std::filesystem::path Files::pathFromQString(QString&& path)
{
    const auto tmp = path.toUtf8();
    return std::filesystem::path{ Misc::StringUtils::stringToU8String(tmp) };
}
