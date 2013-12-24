#include "inisettings.hpp"


#include <QTextStream>
#include <QFile>
#include <QString>
#include <QRegExp>
#include <QDebug>

Wizard::IniSettings::IniSettings()
{
}

Wizard::IniSettings::~IniSettings()
{
}

bool Wizard::IniSettings::readFile(QTextStream &stream)
{
    // Look for a square bracket, "'\\["
    // that has one or more "not nothing" in it, "([^]]+)"
    // and is closed with a square bracket, "\\]"
    QRegExp sectionRe("^\\[([^]]+)\\]");

    // Find any character(s) that is/are not equal sign(s), "[^=]+"
    // followed by an optional whitespace, an equal sign, and another optional whitespace, "\\s*=\\s*"
    // and one or more periods, "(.+)"
    QRegExp keyRe("^([^=]+)\\s*=\\s*(.+)$");

    QString currentSection;

    while (!stream.atEnd())
    {
        QString line(stream.readLine());

        if (line.isEmpty() || line.startsWith(";"))
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

bool Wizard::IniSettings::writeFile(QTextStream &stream)
{
    qDebug() << "test! " << stream.readAll();

     while (!stream.atEnd()) {
         qDebug() << "test! " <<  stream.readLine();
     }

    return true;
}
