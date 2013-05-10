#include "groupblock.hpp"
#include "itemblock.hpp"

CsSettings::GroupBlock::GroupBlock (QWidget* parent)
    : AbstractBlock (parent)
{}

CsSettings::GroupBlock::GroupBlock (bool isVisible, QWidget *parent)
    : AbstractBlock (isVisible, parent)
{}

int CsSettings::GroupBlock::build (GroupBlockDef &def)
{

    if (def.properties.size() == 0)
        return -1;

    int retVal = 0;

    setVisible (def.isVisible);

    mBox->setLayout(createLayout (def.widgetOrientation, true));

    setObjectName (def.title);
    mBox->setTitle (def.title);

    foreach (SettingsItemDef *itemDef, def.properties)
    {
        ItemBlock *block = new ItemBlock (mBox);

        if (block->build (*itemDef) < 0)
        {
            retVal = -2;
            break;
        }

        mItemBlockList << block;
        mBox->layout()->addWidget (block->getGroupBox());

        connect (block, SIGNAL (signalUpdateSetting (const QString &, const QString &)),
                 this, SLOT (slotUpdateSetting (const QString &, const QString &) ));
    }

    return retVal;
}

CsSettings::SettingList *CsSettings::GroupBlock::getSettings()
{
    SettingList *settings = 0;

    foreach (ItemBlock *block, mItemBlockList)
    {
        if (!settings)
            settings = new SettingList();

        settings->append(*(block->getSettings ()));
    }

    return settings;
}

CsSettings::ItemBlock *CsSettings::GroupBlock::getItemBlock (const QString &name, ItemBlockList *blockList)
{
    ItemBlock *retBlock = 0;

    if (!blockList)
        blockList = &mItemBlockList;

    foreach (ItemBlock *block, *blockList)
    {
        if (block->objectName() == name)
        {
            retBlock = block;
            break;
        }
    }

    return retBlock;
}

CsSettings::ItemBlock *CsSettings::GroupBlock::getItemBlock (int index)
{
    ItemBlock *retBlock = 0;

    if (mItemBlockList.size() > index)
        retBlock = mItemBlockList.at(index);

    return retBlock;
}

bool CsSettings::GroupBlock::updateSettings (const SettingMap &settings)
{
    bool success = true;

    //update all non-proxy settings
    foreach (ItemBlock *block, mItemBlockList)
    {
        SettingContainer *setting = settings[block->objectName()];

        if (setting)
        {
            bool success2 = block->update (setting->getValue());
            success = success && success2;
        }
    }

    return success;
}
