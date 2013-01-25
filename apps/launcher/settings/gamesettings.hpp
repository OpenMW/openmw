#ifndef GAMESETTINGS_HPP
#define GAMESETTINGS_HPP

#include "settingsbase.hpp"

class GameSettings : public SettingsBase<QMultiMap<QString, QString>>
{
public:
    GameSettings();
    ~GameSettings();

     bool writeFile(QTextStream &stream);
};

#endif // GAMESETTINGS_HPP
