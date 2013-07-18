#include "datadisplayformatpage.hpp"
#include "groupblock.hpp"
#include "../../model/settings/usersettings.hpp"

CSVSettings::DataDisplayFormatPage::DataDisplayFormatPage(QWidget* parent) :
    AbstractPage("Display Format", parent)
{
    setupUi();
}

CSVSettings::GroupBlockDef *CSVSettings::DataDisplayFormatPage::setupDataDisplay( const QString &title)
{
    GroupBlockDef *statusBlock = new GroupBlockDef(QString(title));

    SettingsItemDef *statusItem = new SettingsItemDef (statusBlock->title, "Icon Only");
    *(statusItem->valueList) << QString("Icon and Text") << QString("Icon Only") << QString("Text Only");

    WidgetDef statusWidget (Widget_RadioButton);
    statusWidget.valueList = statusItem->valueList;

    statusItem->widget = statusWidget;

    statusBlock->settingItems << statusItem;

    statusBlock->isZeroMargin = false;

    return statusBlock;
}


void CSVSettings::DataDisplayFormatPage::setupUi()
{

    mAbstractBlocks << buildBlock<GroupBlock> (setupDataDisplay ("Record Status Display"));
    mAbstractBlocks << buildBlock<GroupBlock> (setupDataDisplay ("Referenceable ID Type Display"));

     foreach (AbstractBlock *block, mAbstractBlocks)
     {
         connect (block, SIGNAL (signalUpdateSetting (const QString &, const QString &)),
                  this, SIGNAL (signalUpdateEditorSetting (const QString &, const QString &)) );
     }

     connect ( this,
               SIGNAL ( signalUpdateEditorSetting (const QString &, const QString &)),
               &(CSMSettings::UserSettings::instance()),
               SIGNAL ( signalUpdateEditorSetting (const QString &, const QString &)));

}

void CSVSettings::DataDisplayFormatPage::initializeWidgets (const CSMSettings::SettingMap &settings)
{
    //iterate each item in each blocks in this section
    //validate the corresponding setting against the defined valuelist if any.
    for (AbstractBlockList::Iterator it_block = mAbstractBlocks.begin();
                                     it_block != mAbstractBlocks.end(); ++it_block)
        (*it_block)->updateSettings (settings);
}
