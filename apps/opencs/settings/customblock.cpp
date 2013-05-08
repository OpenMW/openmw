#include "customblock.hpp"
#include "groupblock.hpp"
#include "itemblock.hpp"
#include "proxyblock.hpp"

CsSettings::CustomBlock::CustomBlock (QWidget *parent) : AbstractBlock (parent)
{
}

int CsSettings::CustomBlock::build(GroupBlockDefList &defList, GroupBlockDefList::iterator *it)
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
            retVal = buildGroupBlock (*(*listIt));
        else
        {
            mGroupList << proxyBlock;
            proxyIt = listIt;
        }
    }

    if (proxyIt != defaultIt)
        retVal = buildProxyBlock (*(*proxyIt), proxyBlock);

    return retVal;
}

CsSettings::GroupBox *CsSettings::CustomBlock::buildGroupBox (CsSettings::OcsWidgetOrientation orientation)
{
    GroupBox *box = new GroupBox (false, mBox);
    QLayout *layout = createLayout (orientation, true, box);

    return box;
}

int CsSettings::CustomBlock::buildGroupBlock(GroupBlockDef &def)
{
    GroupBlock *block = new GroupBlock (getParent());

    mGroupList << block;

    connect (block, SIGNAL (signalUpdateSetting(const QString &, const QString &)),
             this, SLOT (slotUpdateSetting (const QString &, const QString &)));

    return block->build(def);
}

int CsSettings::CustomBlock::buildProxyBlock(GroupBlockDef& def, ProxyBlock *block)
{
    if (def.properties.size() != 1)
        return -1;

    int retVal = block->build(def);

    if (retVal != 0)
        return retVal;

    foreach (QStringList *list, *(def.properties.at(0)->proxyList))
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

CsSettings::SettingList *CsSettings::CustomBlock::getSettings()
{
    SettingList *settings = new SettingList();

    foreach (GroupBlock *block, mGroupList)
    {
        SettingList *groupSettings = block->getSettings();

        if (groupSettings)
            settings->append(*groupSettings);
    }

    return settings;
}

bool CsSettings::CustomBlock::updateSettings (const SettingMap &settings)
{
    bool success = true;

    foreach (GroupBlock *block, mGroupList)
    {
        bool success2 =  block->updateSettings (settings);
        success = success && success2;
    }

    return success;
}
