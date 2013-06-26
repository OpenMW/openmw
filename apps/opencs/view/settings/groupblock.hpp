#ifndef GROUPBLOCK_HPP
#define GROUPBLOCK_HPP

#include <QList>
#include "abstractblock.hpp"

namespace CSVSettings
{
    class ItemBlock;

    /// Base class for group blocks.
    /// Derived block classes should use CustomBlock
    class GroupBlock : public AbstractBlock
    {
        ItemBlockList mItemBlockList;

    public:
        GroupBlock (QWidget* parent = 0);
        GroupBlock (bool isVisible, QWidget *parent = 0);

        /// build the gorup block based on passed definition
        int build (GroupBlockDef *def);

        /// update settings local to the group block
        bool updateSettings (const CSMSettings::SettingMap &settings);

        /// retrieve setting list local to the group block
        CSMSettings::SettingList *getSettings();

        /// retrieve item block by name from the passed list or local list
        ItemBlock *getItemBlock (const QString &name, ItemBlockList *blockList = 0);

        /// retrieve the item block by index from the local list
        ItemBlock *getItemBlock (int index);

    protected:

        /// create block layout based on passed definition
        int buildLayout (GroupBlockDef &def);

    };
}
#endif // GROUPBLOCK_HPP
