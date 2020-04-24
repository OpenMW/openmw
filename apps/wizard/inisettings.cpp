#include "inisettings.hpp"

#include <QDir>

#include <QTextStream>
#include <QFile>
#include <QStringList>
#include <QString>
#include <QRegExp>
#include <QDebug>

Wizard::IniSettings::IniSettings()
{
}

Wizard::IniSettings::~IniSettings()
{
}

QStringList Wizard::IniSettings::findKeys(const QString &text)
{
    QStringList result;

    for (const QString &key : mSettings.keys())
    {

        if (key.startsWith(text))
            result << key;

    }

    return result;
}

bool Wizard::IniSettings::readFile(QTextStream &stream)
{
    // Look for a square bracket, "'\\["
    // that has one or more "not nothing" in it, "([^]]+)"
    // and is closed with a square bracket, "\\]"
    QRegExp sectionRe(QLatin1String("^\\[([^]]+)\\]"));

    // Find any character(s) that is/are not equal sign(s), "[^=]+"
    // followed by an optional whitespace, an equal sign, and another optional whitespace, "\\s*=\\s*"
    // and one or more periods, "(.+)"
    QRegExp keyRe(QLatin1String("^([^=]+)\\s*=\\s*(.+)$"));

    QString currentSection;

    while (!stream.atEnd())
    {
        const QString line(stream.readLine());

        if (line.isEmpty() || line.startsWith(QLatin1Char(';')))
            continue;

        if (sectionRe.exactMatch(line))
        {
            currentSection = sectionRe.cap(1);
        }
        else if (keyRe.indexIn(line) != -1)
        {
            QString key = keyRe.cap(1).trimmed();
            QString value = keyRe.cap(2).trimmed();

            // Append the section, but only if there is one
            if (!currentSection.isEmpty())
                key = currentSection + QLatin1Char('/') + key;

            mSettings[key] = QVariant(value);
        }
    }

    return true;
}

bool Wizard::IniSettings::writeFile(const QString &path, QTextStream &stream)
{
    // Look for a square bracket, "'\\["
    // that has one or more "not nothing" in it, "([^]]+)"
    // and is closed with a square bracket, "\\]"
    QRegExp sectionRe(QLatin1String("^\\[([^]]+)\\]"));

    // Find any character(s) that is/are not equal sign(s), "[^=]+"
    // followed by an optional whitespace, an equal sign, and another optional whitespace, "\\s*=\\s*"
    // and one or more periods, "(.+)"
    QRegExp keyRe(QLatin1String("^([^=]+)\\s*=\\s*(.+)$"));

    const QStringList keys(mSettings.keys());

    QString currentSection;
    QString buffer;

    while (!stream.atEnd()) {

        const QString line(stream.readLine());

        if (line.isEmpty() || line.startsWith(QLatin1Char(';'))) {
            buffer.append(line + QLatin1String("\n"));
            continue;
        }

        if (sectionRe.exactMatch(line)) {
            buffer.append(line + QLatin1String("\n"));
            currentSection = sectionRe.cap(1);
        } else  if (keyRe.indexIn(line) != -1) {
            QString key(keyRe.cap(1).trimmed());
            QString lookupKey(key);

            // Append the section, but only if there is one
            if (!currentSection.isEmpty())
                lookupKey = currentSection + QLatin1Char('/') + key;

            buffer.append(key + QLatin1Char('=') + mSettings[lookupKey].toString() + QLatin1String("\n"));
            mSettings.remove(lookupKey);
        }
    }

    // Add the new settings to the buffer
    QHashIterator<QString, QVariant> i(mSettings);
    while (i.hasNext()) {
        i.next();

        QStringList fullKey(i.key().split(QLatin1Char('/')));
        QString section(fullKey.at(0));
        section.prepend(QLatin1Char('['));
        section.append(QLatin1Char(']'));
        QString key(fullKey.at(1));

        int index = buffer.lastIndexOf(section);
        if (index != -1) {
            // Look for the next section
            index = buffer.indexOf(QLatin1Char('['), index + 1);

            if (index == -1 ) {
                // We are at the last section, append it to the bottom of the file
                buffer.append(QString("\n%1=%2").arg(key, i.value().toString()));
                mSettings.remove(i.key());
                continue;
            } else {
                // Not at last section, add the key at the index
                buffer.insert(index - 1, QString("\n%1=%2").arg(key, i.value().toString()));
                mSettings.remove(i.key());
            }

        } else {
            // Add the section to the end of the file, because it's not found
            buffer.append(QString("\n%1\n").arg(section));
            i.previous();
        }
    }

    // Now we reopen the file, this time we write
    QFile file(path);

    if (file.open(QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text)) {
        QTextStream in(&file);
        in.setCodec(stream.codec());

        // Write the updated buffer to an empty file
        in << buffer;
        file.flush();
        file.close();
    } else {
        return false;
    }

    return true;
}

bool Wizard::IniSettings::parseInx(const QString &path)
{
    QFile file(path);

    if (file.open(QIODevice::ReadOnly))
    {

        const QByteArray data(file.readAll());
        const QByteArray pattern("\x21\x00\x1A\x01\x04\x00\x04\x97\xFF\x06", 10);

        int i = 0;
        while ((i = data.indexOf(pattern, i)) != -1) {

            int next = data.indexOf(pattern, i + 1);
            if (next == -1)
                break;

            QByteArray array(data.mid(i, (next - i)));

            // Skip some invalid entries
            if (array.contains("\x04\x96\xFF")) {
                ++i;
                continue;
            }

            // Remove the pattern from the beginning
            array.remove(0, 12);

            int index = array.indexOf("\x06");
            const QString section(array.left(index));

            // Figure how many characters to read for the key
            int length = array.indexOf("\x06", section.length() + 3) - (section.length() + 3);
            const QString key(array.mid(section.length() + 3, length));

            QString value(array.mid(section.length() + key.length() + 6));

            // Add the value
            setValue(section + QLatin1Char('/') + key, QVariant(value));

            ++i;
        }

        file.close();
    } else {
        qDebug() << "Failed to open INX file: " << path;
        return false;
    }

    return true;
}
