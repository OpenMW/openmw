#ifndef COMPONENTS_FILES_QTCONVERSION_HPP
#define COMPONENTS_FILES_QTCONVERSION_HPP

#include <QString>
#include <filesystem>

namespace Files
{
    QString pathToQString(const std::filesystem::path& path);

    QString pathToQString(std::filesystem::path&& path);

    std::filesystem::path pathFromQString(QStringView path);

    std::filesystem::path pathFromQString(QString&& path);
}

#endif // COMPONENTS_FILES_QTCONVERSION_HPP
