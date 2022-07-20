#include "boolsetting.hpp"

#include <QCheckBox>
#include <QMutexLocker>

#include <components/settings/settings.hpp>

#include "category.hpp"
#include "state.hpp"

CSMPrefs::BoolSetting::BoolSetting (Category *parent,
  QMutex *mutex, const std::string& key, const std::string& label, bool default_)
: Setting (parent, mutex, key, label),  mDefault (default_), mWidget(nullptr)
{}

CSMPrefs::BoolSetting& CSMPrefs::BoolSetting::setTooltip (const std::string& tooltip)
{
    mTooltip = tooltip;
    return *this;
}

std::pair<QWidget *, QWidget *> CSMPrefs::BoolSetting::makeWidgets (QWidget *parent)
{
    mWidget = new QCheckBox (QString::fromUtf8 (getLabel().c_str()), parent);
    mWidget->setCheckState (mDefault ? Qt::Checked : Qt::Unchecked);

    if (!mTooltip.empty())
    {
        QString tooltip = QString::fromUtf8 (mTooltip.c_str());
        mWidget->setToolTip (tooltip);
    }

    connect (mWidget, SIGNAL (stateChanged (int)), this, SLOT (valueChanged (int)));

    return std::make_pair (static_cast<QWidget *> (nullptr), mWidget);
}

void CSMPrefs::BoolSetting::updateWidget()
{
    if (mWidget)
    {
        mWidget->setCheckState(Settings::Manager::getBool(getKey(), getParent()->getKey())
            ? Qt::Checked
            : Qt::Unchecked);
    }
}

void CSMPrefs::BoolSetting::valueChanged (int value)
{
    {
        QMutexLocker lock (getMutex());
        Settings::Manager::setBool (getKey(), getParent()->getKey(), value);
    }

    getParent()->getState()->update (*this);
}
