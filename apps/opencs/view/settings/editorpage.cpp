#include "editorpage.hpp"
#include "groupblock.hpp"
#include "../../model/settings/usersettings.hpp"

CSVSettings::EditorPage::EditorPage(QWidget* parent) :
    AbstractPage("Display Format", parent)
{
    setupUi();
}

CSVSettings::GroupBlockDef *CSVSettings::EditorPage::setupRecordStatusDisplay()
{
    GroupBlockDef *statusBlock = new GroupBlockDef(QString("Record Status Display"));

    SettingsItemDef *statusItem = new SettingsItemDef (statusBlock->title, "Icon and Text");
    *(statusItem->valueList) << QString("Icon and Text") << QString("Icon Only") << QString("Text Only");

    WidgetDef statusWidget (Widget_RadioButton);
    statusWidget.valueList = statusItem->valueList;

    statusItem->widget = statusWidget;

    statusBlock->settingItems << statusItem;

    return statusBlock;
}

void CSVSettings::EditorPage::setupUi()
{

    mAbstractBlocks << buildBlock<GroupBlock>(setupRecordStatusDisplay());

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

void CSVSettings::EditorPage::initializeWidgets (const CSMSettings::DefinitionMap &settings)
{
    //iterate each item in each blocks in this section
    //validate the corresponding setting against the defined valuelist if any.
    for (AbstractBlockList::Iterator it_block = mAbstractBlocks.begin();
                                     it_block != mAbstractBlocks.end(); ++it_block)
        (*it_block)->updateSettings (settings);
}
