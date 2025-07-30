#ifndef OPENMW_COMPONENTS_MISC_UTF8QTEXTSTREAM_HPP
#define OPENMW_COMPONENTS_MISC_UTF8QTEXTSTREAM_HPP

#include <QtGlobal>

#include <QTextStream>

namespace Misc
{
    inline void ensureUtf8Encoding(QTextStream& stream)
    {
        stream.setEncoding(QStringConverter::Utf8);
    }
}
#endif
