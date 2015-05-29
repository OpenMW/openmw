#include "signalhandler.hpp"

#include "../settings/usersettings.hpp"

CSMTools::SignalHandler::SignalHandler(bool extraCheck)
    : mExtraCheck(extraCheck)
{
    connect (&CSMSettings::UserSettings::instance(),
             SIGNAL (userSettingUpdated(const QString &, const QStringList &)),
             this,
             SLOT (updateUserSetting (const QString &, const QStringList &)));
}

void CSMTools::SignalHandler::updateUserSetting (const QString &name, const QStringList &list)
{
    if (name=="verifier/pathgrid-extra-check")
        mExtraCheck = list.at(0) == "true";
}

bool CSMTools::SignalHandler::extraCheck ()
{
    return mExtraCheck;
}
