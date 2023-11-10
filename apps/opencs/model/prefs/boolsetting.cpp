#include "boolsetting.hpp"

#include <QCheckBox>
#include <QMutexLocker>

#include <components/settings/settings.hpp>

#include <apps/opencs/model/prefs/setting.hpp>

#include "category.hpp"
#include "state.hpp"

CSMPrefs::BoolSetting::BoolSetting(
    Category* parent, QMutex* mutex, const std::string& key, const QString& label, bool default_)
    : Setting(parent, mutex, key, label)
    , mDefault(default_)
    , mWidget(nullptr)
{
}

CSMPrefs::BoolSetting& CSMPrefs::BoolSetting::setTooltip(const std::string& tooltip)
{
    mTooltip = tooltip;
    return *this;
}

CSMPrefs::SettingWidgets CSMPrefs::BoolSetting::makeWidgets(QWidget* parent)
{
    mWidget = new QCheckBox(getLabel(), parent);
    mWidget->setCheckState(mDefault ? Qt::Checked : Qt::Unchecked);

    if (!mTooltip.empty())
    {
        QString tooltip = QString::fromUtf8(mTooltip.c_str());
        mWidget->setToolTip(tooltip);
    }

    connect(mWidget, &QCheckBox::stateChanged, this, &BoolSetting::valueChanged);

    return SettingWidgets{ .mLabel = nullptr, .mInput = mWidget, .mLayout = nullptr };
}

void CSMPrefs::BoolSetting::updateWidget()
{
    if (mWidget)
    {
        mWidget->setCheckState(
            Settings::Manager::getBool(getKey(), getParent()->getKey()) ? Qt::Checked : Qt::Unchecked);
    }
}

void CSMPrefs::BoolSetting::valueChanged(int value)
{
    {
        QMutexLocker lock(getMutex());
        Settings::Manager::setBool(getKey(), getParent()->getKey(), value);
    }

    getParent()->getState()->update(*this);
}
