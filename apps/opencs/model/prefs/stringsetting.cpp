
#include "stringsetting.hpp"

#include <QLineEdit>
#include <QMutexLocker>

#include <components/settings/settings.hpp>

#include "category.hpp"
#include "state.hpp"

CSMPrefs::StringSetting::StringSetting (Category *parent,
  QMutex *mutex, const std::string& key, const std::string& label, std::string default_)
: Setting (parent, mutex, key, label),  mDefault (default_), mWidget(nullptr)
{}

CSMPrefs::StringSetting& CSMPrefs::StringSetting::setTooltip (const std::string& tooltip)
{
    mTooltip = tooltip;
    return *this;
}

std::pair<QWidget *, QWidget *> CSMPrefs::StringSetting::makeWidgets (QWidget *parent)
{
    mWidget = new QLineEdit (QString::fromUtf8 (mDefault.c_str()), parent);

    if (!mTooltip.empty())
    {
        QString tooltip = QString::fromUtf8 (mTooltip.c_str());
        mWidget->setToolTip (tooltip);
    }

    connect (mWidget, SIGNAL (textChanged (QString)), this, SLOT (textChanged (QString)));

    return std::make_pair (static_cast<QWidget *> (nullptr), mWidget);
}

void CSMPrefs::StringSetting::updateWidget()
{
    if (mWidget)
    {
        mWidget->setText(QString::fromStdString(Settings::Manager::getString(getKey(), getParent()->getKey())));
    }
}

void CSMPrefs::StringSetting::textChanged (const QString& text)
{
    {
        QMutexLocker lock (getMutex());
        Settings::Manager::setString (getKey(), getParent()->getKey(), text.toStdString());
    }

    getParent()->getState()->update (*this);
}
