
#include "setting.hpp"

#include <QColor>
#include <QMutexLocker>

#include <components/settings/settings.hpp>
#include <components/settings/settingvalue.hpp>

#include "category.hpp"
#include "state.hpp"

QMutex* CSMPrefs::Setting::getMutex()
{
    return mMutex;
}

CSMPrefs::Setting::Setting(
    Category* parent, QMutex* mutex, std::string_view key, const QString& label, Settings::Index& index)
    : QObject(parent->getState())
    , mParent(parent)
    , mMutex(mutex)
    , mKey(key)
    , mLabel(label)
    , mIndex(index)
{
}

const CSMPrefs::Category* CSMPrefs::Setting::getParent() const
{
    return mParent;
}

const std::string& CSMPrefs::Setting::getKey() const
{
    return mKey;
}

QColor CSMPrefs::Setting::toColor() const
{
    // toString() handles lock
    return QColor(QString::fromUtf8(toString().c_str()));
}

bool CSMPrefs::operator==(const Setting& setting, const std::string& key)
{
    std::string fullKey = setting.getParent()->getKey() + "/" + setting.getKey();
    return fullKey == key;
}

bool CSMPrefs::operator==(const std::string& key, const Setting& setting)
{
    return setting == key;
}

bool CSMPrefs::operator!=(const Setting& setting, const std::string& key)
{
    return !(setting == key);
}

bool CSMPrefs::operator!=(const std::string& key, const Setting& setting)
{
    return !(key == setting);
}
