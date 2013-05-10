#ifndef ITEMBLOCK_HPP
#define ITEMBLOCK_HPP

#include "abstractblock.hpp"

namespace CsSettings
{

    class ItemBlock : public AbstractBlock
    {
        SettingsItem *mSetting;
        WidgetList mWidgetList;

    public:

        ItemBlock (QWidget* parent = 0);

        bool updateSettings (const SettingMap &settings) { return false; }

        SettingList *getSettings ();
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
