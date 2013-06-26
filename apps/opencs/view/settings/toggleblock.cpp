#include "toggleblock.hpp"
#include "groupblock.hpp"
#include "groupbox.hpp"
#include "itemblock.hpp"

CSVSettings::ToggleBlock::ToggleBlock(QWidget *parent) :
    CustomBlock(parent)
{}

int CSVSettings::ToggleBlock::build(CustomBlockDef *def)
{
    if (def->blockDefList.size()==0)
        return -1;

    QList<GroupBlockDef *>::Iterator it = def->blockDefList.begin();

    //first def in the list is the def for the toggle block
    GroupBlockDef *toggleDef = *it++;

    if (toggleDef->captions.size() != def->blockDefList.size()-1 )
        return -2;

    if (toggleDef->widgets.size() == 0)
        return -3;

    //create the toogle block UI structure
    QLayout *blockLayout = createLayout (def->blockOrientation, true);
    GroupBox *propertyBox = buildGroupBox (toggleDef->widgetOrientation);

    mBox->setLayout(blockLayout);
    mBox->setTitle (toggleDef->title);

    //build the blocks contained in the def list
    //this manages proxy block construction.
    //Any settings managed by the proxy setting
    //must be included in the blocks defined in the list.
    CustomBlock::build (def->blockDefList, &it);

    for (GroupBlockList::iterator it = mGroupList.begin(); it != mGroupList.end(); ++it)
        propertyBox->layout()->addWidget ((*it)->getGroupBox());

    //build togle widgets, linking them to the settings
    GroupBox *toggleBox = buildToggleWidgets (toggleDef, def->defaultValue);

    blockLayout->addWidget(toggleBox);
    blockLayout->addWidget(propertyBox);
    blockLayout->setAlignment (propertyBox, Qt::AlignRight);

    return 0;
}

CSVSettings::GroupBox *CSVSettings::ToggleBlock::buildToggleWidgets (GroupBlockDef *def, QString &defaultToggle)
{
    GroupBox *box = new GroupBox (false, getParent());

    QLayout *layout = createLayout (def->widgetOrientation, true, static_cast<QWidget *>(box));

    for (int i = 0; i < def->widgets.size(); ++i)
    {
        QString caption = def->captions.at(i);
        WidgetDef *wDef = def->widgets.at(i);

        wDef->caption = caption;
        wDef->widgetAlignment = Align_Left;

        AbstractWidget *widg = buildWidget (caption, *wDef, layout, false);

        GroupBlock *block = mGroupList.at(i);

        //connect widget's update to the property block's enabled status
        connect (widg->widget(), SIGNAL (toggled (bool)), block, SLOT (slotSetEnabled(bool)));

        //enable the default toggle option
        block->getGroupBox()->setEnabled( caption == defaultToggle );

        layout = widg->getLayout();
    }

    return box;
}
