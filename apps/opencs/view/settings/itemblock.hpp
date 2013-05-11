#ifndef ITEMBLOCK_HPP
#define ITEMBLOCK_HPP

#include "abstractblock.hpp"

namespace CSVSettings
{

    class ItemBlock : public AbstractBlock
    {
        CSMSettings::SettingsItem *mSetting;
        WidgetList mWidgetList;

    public:

        ItemBlock (QWidget* parent = 0);

        bool updateSettings (const CSMSettings::SettingMap &settings) { return false; }

        CSMSettings::SettingList *getSettings ();
        QString getValue () const;

        int getSettingCount();
        bool update (const QString &value);

        int build(SettingsItemDef &iDef);

    private:

        void buildItemBlock (SettingsItemDef& iDef);
        void buildItemBlockWidgets (SettingsItemDef& iDef);
        bool updateItem (const QString &);

        bool updateBySignal (const QString &name, const QString &value, bool &doEmit);
    };
}

#endif // ITEMBLOCK_HPP
