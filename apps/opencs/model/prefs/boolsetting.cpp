#include "boolsetting.hpp"

#include <QCheckBox>
#include <QMutexLocker>

#include <components/settings/settings.hpp>

#include <apps/opencs/model/prefs/setting.hpp>

#include "category.hpp"
#include "state.hpp"

CSMPrefs::BoolSetting::BoolSetting(
    Category* parent, QMutex* mutex, std::string_view key, const QString& label, Settings::Index& index)
    : TypedSetting(parent, mutex, key, label, index)
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
    mWidget->setCheckState(getValue() ? Qt::Checked : Qt::Unchecked);

    if (!mTooltip.empty())
    {
        QString tooltip = QString::fromUtf8(mTooltip.c_str());
        mWidget->setToolTip(tooltip);
    }

    connect(mWidget, &QCheckBox::checkStateChanged, this, &BoolSetting::valueChanged);

    return SettingWidgets{ .mLabel = nullptr, .mInput = mWidget };
}

void CSMPrefs::BoolSetting::updateWidget()
{
    if (mWidget)
    {
        mWidget->setCheckState(getValue() ? Qt::Checked : Qt::Unchecked);
    }
}

void CSMPrefs::BoolSetting::valueChanged(int value)
{
    setValue(value != Qt::Unchecked);
    getParent()->getState()->update(*this);
}
