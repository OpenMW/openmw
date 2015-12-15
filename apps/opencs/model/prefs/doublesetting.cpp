
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
  mMin (0), mMax (std::numeric_limits<double>::max()),
  mDefault (default_)
{}

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

    QDoubleSpinBox *widget = new QDoubleSpinBox (parent);
    widget->setRange (mMin, mMax);
    widget->setValue (mDefault);

    if (!mTooltip.empty())
    {
        QString tooltip = QString::fromUtf8 (mTooltip.c_str());
        label->setToolTip (tooltip);
        widget->setToolTip (tooltip);
    }

    connect (widget, SIGNAL (valueChanged (double)), this, SLOT (valueChanged (double)));

    return std::make_pair (label, widget);
}

void CSMPrefs::DoubleSetting::valueChanged (double value)
{
    {
        QMutexLocker lock (getMutex());
        getValues().setFloat (getKey(), getParent()->getKey(), value);
    }

    getParent()->getState()->update (*this);
}
