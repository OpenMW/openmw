#include "signalhandler.hpp"

#include <QMetaObject>

#include "../settings/usersettings.hpp"

CSMTools::SignalHandler::SignalHandler(bool extraCheck)
    : mExtraCheck(extraCheck)
{
    connect (&CSMSettings::UserSettings::instance(),
             SIGNAL (userSettingUpdated(const QString &, const QStringList &)),
             this,
             SLOT (updateUserSetting (const QString &, const QStringList &)));
}

// called from the main thread
void CSMTools::SignalHandler::updateUserSetting (const QString &name, const QStringList &list)
{
    if (name=="verifier/pathgrid-extra-check" && !list.empty())
        QMetaObject::invokeMethod(this, "updateExtraCheck", Qt::AutoConnection, Q_ARG(bool, list.at(0) == "true"));
}

// should be in the operations thread via an event message queue
void CSMTools::SignalHandler::updateExtraCheck (bool extraCheck)
{
    mExtraCheck = extraCheck;
}

// called from the operations thread
bool CSMTools::SignalHandler::extraCheck ()
{
    return mExtraCheck;
}
