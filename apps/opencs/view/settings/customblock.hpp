#ifndef CUSTOMBLOCK_HPP
#define CUSTOMBLOCK_HPP

#include "abstractblock.hpp"

namespace CSVSettings
{

    class ProxyBlock;

    class CustomBlock : public AbstractBlock
    {

    protected:

        GroupBlockList mGroupList;

    public:

        explicit CustomBlock (QWidget *parent = 0);

        bool updateSettings (const CSMSettings::SettingMap &settings);
        CSMSettings::SettingList *getSettings();
        int build (GroupBlockDefList &defList, GroupBlockDefList::Iterator *it = 0);

    protected:

        GroupBox *buildGroupBox (Orientation orientation);

    private:

        int buildGroupBlock(GroupBlockDef &def);
        int buildProxyBlock(GroupBlockDef &def, ProxyBlock *block);
    };
}
#endif // CUSTOMBLOCK_HPP
