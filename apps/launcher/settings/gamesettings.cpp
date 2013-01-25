#include <QTextStream>
#include <QString>
#include <QRegExp>
#include <QMap>

#include <QDebug>

#include "gamesettings.hpp"

GameSettings::GameSettings()
{
}

GameSettings::~GameSettings()
{
}

bool GameSettings::writeFile(QTextStream &stream)
{
    QMap<QString, QString> settings = SettingsBase::getSettings();

    QMapIterator<QString, QString> i(settings);
    while (i.hasNext()) {
        i.next();

        // Quote values with spaces
        if (i.value().contains(" ")) {
            stream << i.key() << "=\"" << i.value() << "\"\n";
        } else {
            stream << i.key() << "=" << i.value() << "\n";
        }

    }
}
