
#include "stringsetting.hpp"

#include <QLabel>
#include <QLineEdit>
#include <QMutexLocker>

#include <components/settings/settings.hpp>

#include <apps/opencs/model/prefs/setting.hpp>

#include "category.hpp"
#include "state.hpp"

CSMPrefs::StringSetting::StringSetting(
    Category* parent, QMutex* mutex, std::string_view key, const QString& label, Settings::Index& index)
    : TypedSetting(parent, mutex, key, label, index)
    , mWidget(nullptr)
{
}

CSMPrefs::StringSetting& CSMPrefs::StringSetting::setTooltip(const std::string& tooltip)
{
    mTooltip = tooltip;
    return *this;
}

CSMPrefs::SettingWidgets CSMPrefs::StringSetting::makeWidgets(QWidget* parent)
{
    QLabel* label = new QLabel(getLabel(), parent);

    mWidget = new QLineEdit(QString::fromStdString(getValue()), parent);
    mWidget->setMinimumWidth(300);

    if (!mTooltip.empty())
    {
        QString tooltip = QString::fromUtf8(mTooltip.c_str());
        label->setToolTip(tooltip);
        mWidget->setToolTip(tooltip);
    }

    connect(mWidget, &QLineEdit::textChanged, this, &StringSetting::textChanged);

    return SettingWidgets{ .mLabel = label, .mInput = mWidget };
}

void CSMPrefs::StringSetting::updateWidget()
{
    if (mWidget)
        mWidget->setText(QString::fromStdString(getValue()));
}

void CSMPrefs::StringSetting::textChanged(const QString& text)
{
    setValue(text.toStdString());
    getParent()->getState()->update(*this);
}
