#include "enumsetting.hpp"

#include <QComboBox>
#include <QLabel>
#include <QMutexLocker>
#include <QString>

#include <algorithm>
#include <memory>

#include <apps/opencs/model/prefs/setting.hpp>

#include <components/settings/settings.hpp>

#include "category.hpp"
#include "state.hpp"

CSMPrefs::EnumValue::EnumValue(const std::string& value, const std::string& tooltip)
    : mValue(value)
    , mTooltip(tooltip)
{
}

CSMPrefs::EnumValue::EnumValue(const char* value)
    : mValue(value)
{
}

CSMPrefs::EnumValues& CSMPrefs::EnumValues::add(const EnumValues& values)
{
    mValues.insert(mValues.end(), values.mValues.begin(), values.mValues.end());
    return *this;
}

CSMPrefs::EnumValues& CSMPrefs::EnumValues::add(const EnumValue& value)
{
    mValues.push_back(value);
    return *this;
}

CSMPrefs::EnumValues& CSMPrefs::EnumValues::add(const std::string& value, const std::string& tooltip)
{
    mValues.emplace_back(value, tooltip);
    return *this;
}

CSMPrefs::EnumSetting::EnumSetting(
    Category* parent, QMutex* mutex, const std::string& key, const QString& label, Settings::Index& index)
    : TypedSetting(parent, mutex, key, label, index)
    , mWidget(nullptr)
{
}

CSMPrefs::EnumSetting& CSMPrefs::EnumSetting::setTooltip(const std::string& tooltip)
{
    mTooltip = tooltip;
    return *this;
}

CSMPrefs::EnumSetting& CSMPrefs::EnumSetting::addValues(const EnumValues& values)
{
    mValues.add(values);
    return *this;
}

CSMPrefs::EnumSetting& CSMPrefs::EnumSetting::addValue(const EnumValue& value)
{
    mValues.add(value);
    return *this;
}

CSMPrefs::EnumSetting& CSMPrefs::EnumSetting::addValue(const std::string& value, const std::string& tooltip)
{
    mValues.add(value, tooltip);
    return *this;
}

CSMPrefs::SettingWidgets CSMPrefs::EnumSetting::makeWidgets(QWidget* parent)
{
    QLabel* label = new QLabel(getLabel(), parent);

    mWidget = new QComboBox(parent);

    size_t index = 0;
    const std::string value = getValue();

    for (size_t i = 0; i < mValues.mValues.size(); ++i)
    {
        if (value == mValues.mValues[i].mValue)
            index = i;

        mWidget->addItem(QString::fromUtf8(mValues.mValues[i].mValue.c_str()));

        if (!mValues.mValues[i].mTooltip.empty())
            mWidget->setItemData(
                static_cast<int>(i), QString::fromUtf8(mValues.mValues[i].mTooltip.c_str()), Qt::ToolTipRole);
    }

    mWidget->setCurrentIndex(static_cast<int>(index));

    if (!mTooltip.empty())
    {
        QString tooltip = QString::fromUtf8(mTooltip.c_str());
        label->setToolTip(tooltip);
    }

    connect(mWidget, qOverload<int>(&QComboBox::currentIndexChanged), this, &EnumSetting::valueChanged);

    return SettingWidgets{ .mLabel = label, .mInput = mWidget };
}

void CSMPrefs::EnumSetting::updateWidget()
{
    if (mWidget)
        mWidget->setCurrentIndex(mWidget->findText(QString::fromStdString(getValue())));
}

void CSMPrefs::EnumSetting::valueChanged(int value)
{
    setValue(mValues.mValues.at(value).mValue);
    getParent()->getState()->update(*this);
}
