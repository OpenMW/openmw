#ifndef ITEMBLOCK_HPP
#define ITEMBLOCK_HPP

#include "abstractblock.hpp"

namespace CSVSettings
{

    class ItemBlock : public AbstractBlock
    {
        //WidgetList mWidgetList;

    public:

        ItemBlock (QWidget* parent = 0);

        /// item blocks encapsulate only one setting
        int getSettingCount();

        /// virtual construction function
        //int build(SettingsItemDef &iDef);

    private:

        /// custom construction function
       // void buildItemBlock (SettingsItemDef& iDef);
       // void buildItemBlockWidgets (SettingsItemDef& iDef);
    };
}

#endif // ITEMBLOCK_HPP
