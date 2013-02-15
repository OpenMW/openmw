#ifndef LAUNCHERSETTINGS_HPP
#define LAUNCHERSETTINGS_HPP

#include "settingsbase.hpp"

class LauncherSettings : public SettingsBase<QMap<QString, QString> >
{
public:
    LauncherSettings();
    ~LauncherSettings();

    QStringList subKeys(const QString &key);
    QStringList values(const QString &key, Qt::MatchFlags flags = Qt::MatchExactly);

    bool writeFile(QTextStream &stream);

};

#endif // LAUNCHERSETTINGS_HPP
