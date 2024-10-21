#ifndef COMPONENTS_VFS_QTCONVERSION_HPP
#define COMPONENTS_VFS_QTCONVERSION_HPP

#include <QString>

#include "pathutil.hpp"

namespace VFS::Path
{
    QString normalizedToQString(NormalizedView path);

    QString normalizedToQString(Normalized&& path);

    Normalized normalizedFromQString(QStringView path);

    Normalized normalizedFromQString(QString&& path);
}

#endif // COMPONENTS_VFS_QTCONVERSION_HPP
