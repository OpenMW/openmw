#include "setting.hpp"
#include <QStringList>

#include <QDebug>

CSMSettings::Setting::Setting (const QString &name, const QString &section,
                               const QString &defaultValue, QObject *parent)
    : QObject (parent), mSectionName (section), mDefaultValue (defaultValue),
      mIsHorizontal (true)
{
    setObjectName (name);
}
