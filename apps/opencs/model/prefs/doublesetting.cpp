
#include "doublesetting.hpp"

#include <limits>

#include <QDoubleSpinBox>
#include <QLabel>
#include <QMutexLocker>

#include <components/settings/settings.hpp>

#include <apps/opencs/model/prefs/setting.hpp>

#include "category.hpp"
#include "state.hpp"

CSMPrefs::DoubleSetting::DoubleSetting(
    Category* parent, QMutex* mutex, std::string_view key, const QString& label, Settings::Index& index)
    : TypedSetting(parent, mutex, key, label, index)
    , mPrecision(2)
    , mMin(0)
    , mMax(std::numeric_limits<double>::max())
    , mWidget(nullptr)
{
}

CSMPrefs::DoubleSetting& CSMPrefs::DoubleSetting::setPrecision(int precision)
{
    mPrecision = precision;
    return *this;
}

CSMPrefs::DoubleSetting& CSMPrefs::DoubleSetting::setRange(double min, double max)
{
    mMin = min;
    mMax = max;
    return *this;
}

CSMPrefs::DoubleSetting& CSMPrefs::DoubleSetting::setMin(double min)
{
    mMin = min;
    return *this;
}

CSMPrefs::DoubleSetting& CSMPrefs::DoubleSetting::setMax(double max)
{
    mMax = max;
    return *this;
}

CSMPrefs::DoubleSetting& CSMPrefs::DoubleSetting::setTooltip(const std::string& tooltip)
{
    mTooltip = tooltip;
    return *this;
}

CSMPrefs::SettingWidgets CSMPrefs::DoubleSetting::makeWidgets(QWidget* parent)
{
    QLabel* label = new QLabel(getLabel(), parent);

    mWidget = new QDoubleSpinBox(parent);
    mWidget->setDecimals(mPrecision);
    mWidget->setRange(mMin, mMax);
    mWidget->setValue(getValue());

    if (!mTooltip.empty())
    {
        QString tooltip = QString::fromUtf8(mTooltip.c_str());
        label->setToolTip(tooltip);
        mWidget->setToolTip(tooltip);
    }

    connect(mWidget, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &DoubleSetting::valueChanged);

    return SettingWidgets{ .mLabel = label, .mInput = mWidget };
}

void CSMPrefs::DoubleSetting::updateWidget()
{
    if (mWidget)
        mWidget->setValue(getValue());
}

void CSMPrefs::DoubleSetting::valueChanged(double value)
{
    setValue(value);
    getParent()->getState()->update(*this);
}
