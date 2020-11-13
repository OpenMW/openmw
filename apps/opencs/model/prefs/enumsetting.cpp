
#include "enumsetting.hpp"

#include <QLabel>
#include <QComboBox>
#include <QMutexLocker>
#include <QString>

#include <components/settings/settings.hpp>

#include "category.hpp"
#include "state.hpp"


CSMPrefs::EnumValue::EnumValue (const std::string& value, const std::string& tooltip)
: mValue (value), mTooltip (tooltip)
{}

CSMPrefs::EnumValue::EnumValue (const char *value)
: mValue (value)
{}


CSMPrefs::EnumValues& CSMPrefs::EnumValues::add (const EnumValues& values)
{
    mValues.insert (mValues.end(), values.mValues.begin(), values.mValues.end());
    return *this;
}

CSMPrefs::EnumValues& CSMPrefs::EnumValues::add (const EnumValue& value)
{
    mValues.push_back (value);
    return *this;
}

CSMPrefs::EnumValues& CSMPrefs::EnumValues::add (const std::string& value, const std::string& tooltip)
{
    mValues.emplace_back(value, tooltip);
    return *this;
}


CSMPrefs::EnumSetting::EnumSetting (Category *parent, Settings::Manager *values,
  QMutex *mutex, const std::string& key, const std::string& label, const EnumValue& default_)
: Setting (parent, values, mutex, key, label), mDefault (default_), mWidget(nullptr)
{}

CSMPrefs::EnumSetting& CSMPrefs::EnumSetting::setTooltip (const std::string& tooltip)
{
    mTooltip = tooltip;
    return *this;
}

CSMPrefs::EnumSetting& CSMPrefs::EnumSetting::addValues (const EnumValues& values)
{
    mValues.add (values);
    return *this;
}

CSMPrefs::EnumSetting& CSMPrefs::EnumSetting::addValue (const EnumValue& value)
{
    mValues.add (value);
    return *this;
}

CSMPrefs::EnumSetting& CSMPrefs::EnumSetting::addValue (const std::string& value, const std::string& tooltip)
{
    mValues.add (value, tooltip);
    return *this;
}

std::pair<QWidget *, QWidget *> CSMPrefs::EnumSetting::makeWidgets (QWidget *parent)
{
    QLabel *label = new QLabel (QString::fromUtf8 (getLabel().c_str()), parent);

    mWidget = new QComboBox (parent);

    int index = 0;

    for (int i=0; i<static_cast<int> (mValues.mValues.size()); ++i)
    {
        if (mDefault.mValue==mValues.mValues[i].mValue)
            index = i;

        mWidget->addItem (QString::fromUtf8 (mValues.mValues[i].mValue.c_str()));

        if (!mValues.mValues[i].mTooltip.empty())
            mWidget->setItemData (i, QString::fromUtf8 (mValues.mValues[i].mTooltip.c_str()),
                Qt::ToolTipRole);
    }

    mWidget->setCurrentIndex (index);

    if (!mTooltip.empty())
    {
        QString tooltip = QString::fromUtf8 (mTooltip.c_str());
        label->setToolTip (tooltip);
    }

    connect (mWidget, SIGNAL (currentIndexChanged (int)), this, SLOT (valueChanged (int)));

    return std::make_pair (label, mWidget);
}

void CSMPrefs::EnumSetting::updateWidget()
{
    if (mWidget)
    {
        int index = mWidget->findText(QString::fromStdString
            (getValues().getString(getKey(), getParent()->getKey())));

        mWidget->setCurrentIndex(index);
    }
}

void CSMPrefs::EnumSetting::valueChanged (int value)
{
    {
        QMutexLocker lock (getMutex());
        getValues().setString (getKey(), getParent()->getKey(), mValues.mValues.at (value).mValue);
    }

    getParent()->getState()->update (*this);
}
