#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QAbstractItemModel>
#include <QGroupBox>
#include <QDataWidgetMapper>

#include "view.hpp"
#include "support.hpp"
#include "../../model/settings/setting.hpp"
#include "settingbox.hpp"

#include <QDebug>

CSVSettings::View::View(QAbstractItemModel *model,
                        const CSMSettings::Setting *setting,
                        QWidget *parent)

    : mModel (model), QWidget(parent) //, mIsMultiValue (setting->isMultiValue)
      //mValueList (setting->valueList)
{/*
    mViewFrame = new SettingBox(true, setting->settingName, parent);
    mViewFrame->setFlat (true);

    if (setting->isHorizontal)
        mViewFrame->setHLayout();
    else
        mViewFrame->setVLayout();

    setObjectName (setting->settingName);*/
}

bool CSVSettings::View::isMultiValue() const
{
    return mIsMultiValue;
}

QStringList CSVSettings::View::valueList() const
{
    return mValueList;
}
