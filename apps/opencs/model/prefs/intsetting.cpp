
#include "intsetting.hpp"

#include <limits>

#include <QLabel>
#include <QMutexLocker>
#include <QSpinBox>

#include <components/settings/settings.hpp>

#include <apps/opencs/model/prefs/setting.hpp>

#include "category.hpp"
#include "state.hpp"

CSMPrefs::IntSetting::IntSetting(
    Category* parent, QMutex* mutex, std::string_view key, const QString& label, Settings::Index& index)
    : TypedSetting(parent, mutex, key, label, index)
    , mMin(0)
    , mMax(std::numeric_limits<int>::max())
    , mWidget(nullptr)
{
}

CSMPrefs::IntSetting& CSMPrefs::IntSetting::setRange(int min, int max)
{
    mMin = min;
    mMax = max;
    return *this;
}

CSMPrefs::IntSetting& CSMPrefs::IntSetting::setMin(int min)
{
    mMin = min;
    return *this;
}

CSMPrefs::IntSetting& CSMPrefs::IntSetting::setMax(int max)
{
    mMax = max;
    return *this;
}

CSMPrefs::IntSetting& CSMPrefs::IntSetting::setTooltip(const std::string& tooltip)
{
    mTooltip = tooltip;
    return *this;
}

CSMPrefs::SettingWidgets CSMPrefs::IntSetting::makeWidgets(QWidget* parent)
{
    QLabel* label = new QLabel(getLabel(), parent);

    mWidget = new QSpinBox(parent);
    mWidget->setRange(mMin, mMax);
    mWidget->setValue(getValue());

    if (!mTooltip.empty())
    {
        QString tooltip = QString::fromUtf8(mTooltip.c_str());
        label->setToolTip(tooltip);
        mWidget->setToolTip(tooltip);
    }

    connect(mWidget, qOverload<int>(&QSpinBox::valueChanged), this, &IntSetting::valueChanged);

    return SettingWidgets{ .mLabel = label, .mInput = mWidget };
}

void CSMPrefs::IntSetting::updateWidget()
{
    if (mWidget)
        mWidget->setValue(getValue());
}

void CSMPrefs::IntSetting::valueChanged(int value)
{
    setValue(value);
    getParent()->getState()->update(*this);
}
