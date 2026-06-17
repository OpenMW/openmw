#include "qtconversion.hpp"

#include <components/misc/strings/conversion.hpp>

#include <string_view>

QString VFS::Path::normalizedToQString(NormalizedView path)
{
    return QString::fromUtf8(path.value().data(), path.value().size());
}

QString VFS::Path::normalizedToQString(Normalized&& path)
{
    return QString::fromUtf8(path.value().data(), path.value().size());
}

VFS::Path::Normalized VFS::Path::normalizedFromQString(QStringView path)
{
    const QByteArray tmp = path.toUtf8();
    return Normalized(std::string_view(tmp.constData(), tmp.size()));
}

VFS::Path::Normalized VFS::Path::normalizedFromQString(QString&& path)
{
    const QByteArray tmp = std::move(path).toUtf8();
    return Normalized(std::string_view(tmp.constData(), tmp.size()));
}
