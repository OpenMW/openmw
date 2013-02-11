#ifndef LAUNCHERSETTINGS_HPP
#define LAUNCHERSETTINGS_HPP

#include "settingsbase.hpp"

class LauncherSettings : public SettingsBase<QMap<QString, QString>>
{
public:
    LauncherSettings();
    ~LauncherSettings();

    bool writeFile(QTextStream &stream);

};

#endif // LAUNCHERSETTINGS_HPP
