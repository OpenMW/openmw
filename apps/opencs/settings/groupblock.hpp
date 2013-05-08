#ifndef GROUPBLOCK_HPP
#define GROUPBLOCK_HPP

#include <QList>
#include "abstractblock.hpp"

namespace CsSettings
{
    class ItemBlock;

    class GroupBlock : public AbstractBlock
    {
        ItemBlockList mItemBlockList;

    public:
        GroupBlock (QWidget* parent = 0);
        GroupBlock (bool isVisible, QWidget *parent = 0);

        int build (GroupBlockDef &def);

        bool updateSettings (const SettingMap &settings);

        SettingList *getSettings();
        ItemBlock *getItemBlock (const QString &name, ItemBlockList *blockList = 0);
        ItemBlock *getItemBlock (int index);

    protected:
        int buildLayout (GroupBlockDef &def);

    };
}
#endif // GROUPBLOCK_HPP
