
#include "intsetting.hpp"

#include <limits>

#include <QLabel>
#include <QSpinBox>
#include <QMutexLocker>

#include <components/settings/settings.hpp>

#include "category.hpp"
#include "state.hpp"

CSMPrefs::IntSetting::IntSetting (Category *parent, Settings::Manager *values,
  QMutex *mutex, const std::string& key, const std::string& label, int default_)
: Setting (parent, values, mutex, key, label), mMin (0), mMax (std::numeric_limits<int>::max()),
  mDefault (default_)
{}

CSMPrefs::IntSetting& CSMPrefs::IntSetting::setRange (int min, int max)
{
    mMin = min;
    mMax = max;
    return *this;
}

CSMPrefs::IntSetting& CSMPrefs::IntSetting::setMin (int min)
{
    mMin = min;
    return *this;
}

CSMPrefs::IntSetting& CSMPrefs::IntSetting::setMax (int max)
{
    mMax = max;
    return *this;
}

CSMPrefs::IntSetting& CSMPrefs::IntSetting::setTooltip (const std::string& tooltip)
{
    mTooltip = tooltip;
    return *this;
}

std::pair<QWidget *, QWidget *> CSMPrefs::IntSetting::makeWidgets (QWidget *parent)
{
    QLabel *label = new QLabel (QString::fromUtf8 (getLabel().c_str()), parent);

    QSpinBox *widget = new QSpinBox (parent);
    widget->setRange (mMin, mMax);
    widget->setValue (mDefault);

    if (!mTooltip.empty())
    {
        QString tooltip = QString::fromUtf8 (mTooltip.c_str());
        label->setToolTip (tooltip);
        widget->setToolTip (tooltip);
    }

    connect (widget, SIGNAL (valueChanged (int)), this, SLOT (valueChanged (int)));

    return std::make_pair (label, widget);
}

void CSMPrefs::IntSetting::valueChanged (int value)
{
    {
        QMutexLocker lock (getMutex());
        getValues().setInt (getKey(), getParent()->getKey(), value);
    }

    getParent()->getState()->update (*this);
}
