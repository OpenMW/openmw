
#include "setting.hpp"

#include <QColor>
#include <QMutexLocker>

#include <components/settings/settings.hpp>

#include "category.hpp"
#include "state.hpp"

QMutex *CSMPrefs::Setting::getMutex()
{
    return mMutex;
}

CSMPrefs::Setting::Setting (Category *parent, QMutex *mutex,
    const std::string& key, const std::string& label)
: QObject (parent->getState()), mParent (parent), mMutex (mutex), mKey (key), mLabel (label)
{}

CSMPrefs::Setting:: ~Setting() {}

std::pair<QWidget *, QWidget *> CSMPrefs::Setting::makeWidgets (QWidget *parent)
{
    return std::pair<QWidget *, QWidget *> (0, 0);
}

void CSMPrefs::Setting::updateWidget()
{
}

const CSMPrefs::Category *CSMPrefs::Setting::getParent() const
{
    return mParent;
}

const std::string& CSMPrefs::Setting::getKey() const
{
    return mKey;
}

const std::string& CSMPrefs::Setting::getLabel() const
{
    return mLabel;
}

int CSMPrefs::Setting::toInt() const
{
    QMutexLocker lock (mMutex);
    return Settings::Manager::getInt (mKey, mParent->getKey());
}

double CSMPrefs::Setting::toDouble() const
{
    QMutexLocker lock (mMutex);
    return Settings::Manager::getFloat (mKey, mParent->getKey());
}

std::string CSMPrefs::Setting::toString() const
{
    QMutexLocker lock (mMutex);
    return Settings::Manager::getString (mKey, mParent->getKey());
}

bool CSMPrefs::Setting::isTrue() const
{
    QMutexLocker lock (mMutex);
    return Settings::Manager::getBool (mKey, mParent->getKey());
}

QColor CSMPrefs::Setting::toColor() const
{
    // toString() handles lock
    return QColor (QString::fromUtf8 (toString().c_str()));
}

bool CSMPrefs::operator== (const Setting& setting, const std::string& key)
{
    std::string fullKey = setting.getParent()->getKey() + "/" + setting.getKey();
    return fullKey==key;
}

bool CSMPrefs::operator== (const std::string& key, const Setting& setting)
{
    return setting==key;
}

bool CSMPrefs::operator!= (const Setting& setting, const std::string& key)
{
    return !(setting==key);
}

bool CSMPrefs::operator!= (const std::string& key, const Setting& setting)
{
    return !(key==setting);
}
