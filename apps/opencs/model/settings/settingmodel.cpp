#include "settingmodel.hpp"
#include "support.hpp"
#include <math.h>
CSMSettings::SettingModel::SettingModel(QObject *parent) :
    QStandardItemModel(0, sPropertyColumnCount, parent)
{
}

CSMSettings::Setting *CSMSettings::SettingModel::setting(int row)
{
    if (row < 0 || row >= rowCount())
        return 0;

    Setting *setting = new Setting(this);

    for (int i = 0; i < sSettingPropertyCount; i++)
        setting->setRowItem(i, item(row, i));

    return setting;
}

CSMSettings::Setting *CSMSettings::SettingModel::declareSetting
                    (SettingType typ, const QString &name, const QString &page,
                     const QString &defaultValue)
{

    if ((typ mod 2) == 0)
    {

    }
    //ensure we're not creating a duplicate setting with the same page, name,
    //and view type
    for (int i = 0; i < rowCount(); i++)
    {
        Setting *setting = row(i);

        if (setting->page() == page)
        {
            if (setting->viewType() == typ)
            {
                if (setting->name() == name)
                {
                    return 0;
                }
            }
        }
    }

    // return a new setting
    return new Setting(this);
}

SettingItem * CSMSettings::SettingModel::item(int row, int column) const
{
    return static_cast<SettingItem *>(QStandardItemModel::item(row, column));
}
