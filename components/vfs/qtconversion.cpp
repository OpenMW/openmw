
#include "qtconversion.hpp"

#include <components/misc/strings/conversion.hpp>

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
    const auto tmp = path.toUtf8();
    return Normalized{ tmp };
}

VFS::Path::Normalized VFS::Path::normalizedFromQString(QString&& path)
{
    const auto tmp = path.toUtf8();
    return Normalized{ tmp };
}
