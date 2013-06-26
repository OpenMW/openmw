#ifndef CUSTOMBLOCK_HPP
#define CUSTOMBLOCK_HPP

#include "abstractblock.hpp"

namespace CSVSettings
{

    class ProxyBlock;

    /// Base class for customized user preference setting blocks
    /// Special block classes should be derived from CustomBlock
    class CustomBlock : public AbstractBlock
    {

    protected:

        GroupBlockList mGroupList;

    public:

        explicit CustomBlock (QWidget *parent = 0);

        /// Update settings local to the block
        bool updateSettings (const CSMSettings::SettingMap &settings);

        /// Retrieve settings local to the block
        CSMSettings::SettingList *getSettings();

        /// construct the block using the passed definition
        int build (GroupBlockDefList &defList, GroupBlockDefList::Iterator *it = 0);

    protected:

        /// construct the block groupbox
        GroupBox *buildGroupBox (Orientation orientation);

    private:

        /// Construction function for creating a standard GroupBlock child
        int buildGroupBlock(GroupBlockDef *def);

        /// Construction function for creating a standard ProxyBlock child
        int buildProxyBlock(GroupBlockDef *def, ProxyBlock *block);
    };
}
#endif // CUSTOMBLOCK_HPP
