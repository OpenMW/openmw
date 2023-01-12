#ifndef MISC_UTF8QTEXTSTREAM_HPP
#define MISC_UTF8QTEXTSTREAM_HPP

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QTextCodec>
#endif
#include <QTextStream>

namespace
{
    void ensureUtf8Encoding(QTextStream& stream)
    {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        stream.setCodec(QTextCodec::codecForName("UTF-8"));
#else
        stream.setEncoding(QStringConverter::Utf8);
#endif
    }
}
#endif
