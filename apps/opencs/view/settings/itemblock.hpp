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

        /// pure virtual function not implemented
        bool updateSettings (const CSMSettings::SettingMap &settings) { return false; }

        CSMSettings::SettingList *getSettings ();

        QString getValue () const;

        /// item blocks encapsulate only one setting
        int getSettingCount();

        /// update setting value and corresponding widget
        bool update (const QString &value);

        /// virtual construction function
        int build(SettingsItemDef &iDef);

    private:

        /// custom construction function
        void buildItemBlock (SettingsItemDef& iDef);
        void buildItemBlockWidgets (SettingsItemDef& iDef);

        /// update the setting value
        bool updateItem (const QString &);

        /// callback function triggered when update to application level is signalled
        bool updateBySignal (const QString &name, const QString &value, bool &doEmit);
    };
}

#endif // ITEMBLOCK_HPP
