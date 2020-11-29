
#include "doublesetting.hpp"

#include <limits>

#include <QLabel>
#include <QDoubleSpinBox>
#include <QMutexLocker>

#include <components/settings/settings.hpp>

#include "category.hpp"
#include "state.hpp"

CSMPrefs::DoubleSetting::DoubleSetting (Category *parent, Settings::Manager *values,
  QMutex *mutex, const std::string& key, const std::string& label, double default_)
: Setting (parent, values, mutex, key, label),
  mPrecision(2), mMin (0), mMax (std::numeric_limits<double>::max()),
  mDefault (default_), mWidget(nullptr)
{}

CSMPrefs::DoubleSetting& CSMPrefs::DoubleSetting::setPrecision(int precision)
{
    mPrecision = precision;
    return *this;
}

CSMPrefs::DoubleSetting& CSMPrefs::DoubleSetting::setRange (double min, double max)
{
    mMin = min;
    mMax = max;
    return *this;
}

CSMPrefs::DoubleSetting& CSMPrefs::DoubleSetting::setMin (double min)
{
    mMin = min;
    return *this;
}

CSMPrefs::DoubleSetting& CSMPrefs::DoubleSetting::setMax (double max)
{
    mMax = max;
    return *this;
}

CSMPrefs::DoubleSetting& CSMPrefs::DoubleSetting::setTooltip (const std::string& tooltip)
{
    mTooltip = tooltip;
    return *this;
}

std::pair<QWidget *, QWidget *> CSMPrefs::DoubleSetting::makeWidgets (QWidget *parent)
{
    QLabel *label = new QLabel (QString::fromUtf8 (getLabel().c_str()), parent);

    mWidget = new QDoubleSpinBox (parent);
    mWidget->setDecimals(mPrecision);
    mWidget->setRange (mMin, mMax);
    mWidget->setValue (mDefault);

    if (!mTooltip.empty())
    {
        QString tooltip = QString::fromUtf8 (mTooltip.c_str());
        label->setToolTip (tooltip);
        mWidget->setToolTip (tooltip);
    }

    connect (mWidget, SIGNAL (valueChanged (double)), this, SLOT (valueChanged (double)));

    return std::make_pair (label, mWidget);
}

void CSMPrefs::DoubleSetting::updateWidget()
{
    if (mWidget)
    {
        mWidget->setValue(getValues().getFloat(getKey(), getParent()->getKey()));
    }
}

void CSMPrefs::DoubleSetting::valueChanged (double value)
{
    {
        QMutexLocker lock (getMutex());
        getValues().setFloat (getKey(), getParent()->getKey(), value);
    }

    getParent()->getState()->update (*this);
}
