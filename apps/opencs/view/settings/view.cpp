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

CSVSettings::View::View(const CSMSettings::Setting &setting,
                        CSMSettings::Adapter *adapter,
                        QWidget *parent)

    : mModel (adapter), QWidget(parent)
{
    mViewFrame = new SettingBox(true, setting.name(), parent);
    mViewFrame->setFlat (true);

        mViewFrame->setHLayout();


        setObjectName (setting.name());
}

bool CSVSettings::View::isMultiValue() const
{
    return mIsMultiValue;
}

QStringList CSVSettings::View::valueList() const
{
    return mValueList;
}
