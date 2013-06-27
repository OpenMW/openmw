#include "customblock.hpp"
#include "groupblock.hpp"
#include "itemblock.hpp"
#include "proxyblock.hpp"

CSVSettings::CustomBlock::CustomBlock (QWidget *parent) : AbstractBlock (parent)
{
}

int CSVSettings::CustomBlock::build(GroupBlockDefList &defList, GroupBlockDefList::iterator *it)
{
    int retVal = 0;

    GroupBlockDefList::iterator defaultIt;
    GroupBlockDefList::iterator listIt = defList.begin();
    GroupBlockDefList::iterator proxyIt = defaultIt;

    if (it)
        listIt = *it;

    ProxyBlock *proxyBlock = new ProxyBlock(getParent());

    for (; listIt != defList.end(); ++listIt)
    {
        if (!(*listIt)->isProxy)
            retVal = buildGroupBlock (*listIt);
        else
        {
            mGroupList << proxyBlock;
            proxyIt = listIt;
        }
    }

    if (proxyIt != defaultIt)
        retVal = buildProxyBlock (*proxyIt, proxyBlock);

    return retVal;
}

CSVSettings::GroupBox *CSVSettings::CustomBlock::buildGroupBox (Orientation orientation)
{
    GroupBox *box = new GroupBox (false, mBox);
    createLayout (orientation, true, box);

    return box;
}

int CSVSettings::CustomBlock::buildGroupBlock(GroupBlockDef *def)
{
    GroupBlock *block = new GroupBlock (getParent());

    mGroupList << block;

    connect (block, SIGNAL (signalUpdateSetting(const QString &, const QString &)),
             this, SLOT (slotUpdateSetting (const QString &, const QString &)));

    return block->build(def);
}

int CSVSettings::CustomBlock::buildProxyBlock(GroupBlockDef *def, ProxyBlock *block)
{
    if (def->settingItems.size() != 1)
        return -1;

    int retVal = block->build(def);

    if (retVal != 0)
        return retVal;

    // The first settingItem is the proxy setting, containing the list of settings bound to it.
    foreach (QStringList *list, *(def->settingItems.at(0)->proxyList))
    {
        QString proxiedBlockName = list->at(0);

        //iterate each group in the custom block, matching it to each proxied setting
        //and connecting it appropriately
        foreach (GroupBlock *groupBlock, mGroupList)
        {
            ItemBlock *proxiedBlock = groupBlock->getItemBlock (proxiedBlockName);

            if (proxiedBlock)
            {
                block->addSetting(proxiedBlock, list);

                //connect the proxy block's update signal to the custom block's slot
                connect (block, SIGNAL (signalUpdateSetting (const QString &, const QString &)),
                         this, SLOT (slotUpdateSetting (const QString &, const QString &)));
            }
        }
    }

    return 0;
}

CSMSettings::SettingList *CSVSettings::CustomBlock::getSettings()
{
    CSMSettings::SettingList *settings = new CSMSettings::SettingList();

    foreach (GroupBlock *block, mGroupList)
    {
        CSMSettings::SettingList *groupSettings = block->getSettings();

        if (groupSettings)
            settings->append(*groupSettings);
    }

    return settings;
}

bool CSVSettings::CustomBlock::updateSettings (const CSMSettings::SettingMap &settings)
{
    bool success = true;

    foreach (GroupBlock *block, mGroupList)
    {
        bool success2 =  block->updateSettings (settings);
        success = success && success2;
    }

    return success;
}
