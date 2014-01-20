#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QAbstractItemModel>
#include <QGroupBox>

#include "view.hpp"
#include "support.hpp"
#include "../../model/settings/setting.hpp"

#include <QDebug>

CSVSettings::View::View(QAbstractItemModel *model,
                        const CSMSettings::Setting *setting, QWidget *parent)

    : mModel (model), mSetting (setting), QWidget(parent)
{
    mViewFrame = new QGroupBox(setting->settingName, parent);

    if (mSetting->isHorizontal)
        mViewFrame->setLayout (new QHBoxLayout());
    else
        mViewFrame->setLayout (new QVBoxLayout());

    mViewFrame->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    setObjectName (mSetting->settingName);
}

bool CSVSettings::View::isMultiValue() const
{
    return mSetting->isMultiValue;
}

QStringList CSVSettings::View::valueList() const
{
    return mSetting->valueList;
}

void CSVSettings::View::slotMapperChanged()
{
    qDebug() << "mapper changed!";
}
