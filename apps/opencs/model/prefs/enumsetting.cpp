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

CSMPrefs::EnumSetting::EnumSetting(Category* parent, QMutex* mutex, std::string_view key, const QString& label,
    std::span<const EnumValueView> values, Settings::Index& index)
    : TypedSetting(parent, mutex, key, label, index)
    , mValues(values)
    , mWidget(nullptr)
{
}

CSMPrefs::EnumSetting& CSMPrefs::EnumSetting::setTooltip(const std::string& tooltip)
{
    mTooltip = tooltip;
    return *this;
}

CSMPrefs::SettingWidgets CSMPrefs::EnumSetting::makeWidgets(QWidget* parent)
{
    QLabel* label = new QLabel(getLabel(), parent);

    mWidget = new QComboBox(parent);

    for (std::size_t i = 0; i < mValues.size(); ++i)
    {
        const EnumValueView& v = mValues[i];

        mWidget->addItem(QString::fromUtf8(v.mValue.data(), static_cast<int>(v.mValue.size())));

        if (!v.mTooltip.empty())
            mWidget->setItemData(static_cast<int>(i),
                QString::fromUtf8(v.mTooltip.data(), static_cast<int>(v.mTooltip.size())), Qt::ToolTipRole);
    }

    const std::string value = getValue();
    const std::size_t index = std::find_if(mValues.begin(), mValues.end(), [&](const EnumValueView& v) {
        return v.mValue == value;
    }) - mValues.begin();

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
    if (value < 0 || static_cast<std::size_t>(value) >= mValues.size())
        throw std::logic_error("Invalid enum setting \"" + getKey() + "\" value index: " + std::to_string(value));

    setValue(std::string(mValues[value].mValue));
    getParent()->getState()->update(*this);
}
