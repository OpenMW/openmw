
#include "boolsetting.hpp"

#include <QCheckBox>
#include <QMutexLocker>

#include <components/settings/settings.hpp>

#include "category.hpp"
#include "state.hpp"

CSMPrefs::BoolSetting::BoolSetting (Category *parent, Settings::Manager *values,
  QMutex *mutex, const std::string& key, const std::string& label, bool default_)
: Setting (parent, values, mutex, key, label),  mDefault (default_)
{}

CSMPrefs::BoolSetting& CSMPrefs::BoolSetting::setTooltip (const std::string& tooltip)
{
    mTooltip = tooltip;
    return *this;
}

std::pair<QWidget *, QWidget *> CSMPrefs::BoolSetting::makeWidgets (QWidget *parent)
{
    QCheckBox *widget = new QCheckBox (QString::fromUtf8 (getLabel().c_str()), parent);
    widget->setCheckState (mDefault ? Qt::Checked : Qt::Unchecked);

    if (!mTooltip.empty())
    {
        QString tooltip = QString::fromUtf8 (mTooltip.c_str());
        widget->setToolTip (tooltip);
    }

    connect (widget, SIGNAL (stateChanged (int)), this, SLOT (valueChanged (int)));

    return std::make_pair (static_cast<QWidget *> (0), widget);
}

void CSMPrefs::BoolSetting::valueChanged (int value)
{
    {
        QMutexLocker lock (getMutex());
        getValues().setBool (getKey(), getParent()->getKey(), value);
    }

    getParent()->getState()->update (*this);
}
