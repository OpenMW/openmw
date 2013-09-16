#include "groupblock.hpp"
#include "itemblock.hpp"

CSVSettings::GroupBlock::GroupBlock (QWidget* parent)
    : AbstractBlock (parent)
{}

CSVSettings::GroupBlock::GroupBlock (bool isVisible, QWidget *parent)
    : AbstractBlock (isVisible, parent)
{}
/*
int CSVSettings::GroupBlock::build (GroupBlockDef *def)
{

    if (def->settingItems.size() == 0)
        return -1;

    int retVal = 0;

    setVisible (def->isVisible);

    mBox->setLayout(createLayout (def->widgetOrientation));

    setObjectName (def->title);
    mBox->setTitle (def->title);

    foreach (SettingsItemDef *itemDef, def->settingItems)
    {
        ItemBlock *block = new ItemBlock (mBox);

        if (block->build (*itemDef) < 0)
        {
            retVal = -2;
            break;
        }

        mItemBlockList << block;
        mBox->layout()->addWidget (block->getGroupBox());
    }

    return retVal;
}

CSVSettings::ItemBlock *CSVSettings::GroupBlock::getItemBlock (const QString &name, ItemBlockList *blockList)
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

CSVSettings::ItemBlock *CSVSettings::GroupBlock::getItemBlock (int index)
{
    ItemBlock *retBlock = 0;

    if (mItemBlockList.size() > index)
        retBlock = mItemBlockList.at(index);

    return retBlock;
}
*/
