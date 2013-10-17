#include "windowpage.hpp"

#include <QList>
#include <QListView>
#include <QGroupBox>
#include <QRadioButton>
#include <QDockWidget>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QStyle>

#ifdef Q_OS_MAC
#include <QPlastiqueStyle>
#endif

#include "../../model/settings/usersettings.hpp"
#include "groupblock.hpp"
#include "toggleblock.hpp"
#include "../../view/settings/abstractblock.hpp"

CSVSettings::WindowPage::WindowPage(QWidget *parent):
    AbstractPage("Window Size", parent)
{
    // Hacks to get the stylesheet look properly
#ifdef Q_OS_MAC
    QPlastiqueStyle *style = new QPlastiqueStyle;
    //profilesComboBox->setStyle(style);
#endif

    setupUi();
}

CSVSettings::GroupBlockDef * CSVSettings::WindowPage::buildDefinedWindowSize()
{
    GroupBlockDef *block = new GroupBlockDef ( "Defined Size");

    SettingsItemDef *widthByHeightItem = new SettingsItemDef ("Window Size", "640x480");
    WidgetDef widthByHeightWidget = WidgetDef (Widget_ComboBox);
    widthByHeightWidget.widgetWidth = 90;
    *(widthByHeightItem->valueList) << "640x480" << "800x600" << "1024x768" << "1440x900";

    QStringList *widthProxy = new QStringList;
    QStringList *heightProxy = new QStringList;

    (*widthProxy) << "Width" << "640" << "800" << "1024" << "1440";
    (*heightProxy) << "Height" << "480" << "600" << "768" << "900";

    *(widthByHeightItem->proxyList) << widthProxy << heightProxy;

    widthByHeightItem->widget = widthByHeightWidget;

    block->settingItems << widthByHeightItem;
    block->isProxy = true;
    block->isVisible = false;

    return block;
}

CSVSettings::GroupBlockDef *CSVSettings::WindowPage::buildCustomWindowSize()
{
    GroupBlockDef *block = new GroupBlockDef ("Custom Size");

    //custom width
    SettingsItemDef *widthItem = new SettingsItemDef ("Width", "640");
    widthItem->widget = WidgetDef (Widget_LineEdit);
    widthItem->widget.widgetWidth = 45;
    widthItem->widget.inputMask = "9999";

    //custom height
    SettingsItemDef *heightItem = new SettingsItemDef ("Height", "480");
    heightItem->widget = WidgetDef (Widget_LineEdit);
    heightItem->widget.widgetWidth = 45;
    heightItem->widget.caption = "x";
    heightItem->widget.inputMask = "9999";

    block->settingItems << widthItem << heightItem;
    block->widgetOrientation = Orient_Horizontal;
    block->isVisible = false;

    return block;
}

CSVSettings::GroupBlockDef *CSVSettings::WindowPage::buildWindowSizeToggle()
{
    GroupBlockDef *block = new GroupBlockDef (objectName());

    // window size toggle
    block->captions << "Pre-Defined" << "Custom";
    block->widgetOrientation = Orient_Vertical;
    block->isVisible = false;

    //define a widget for each group in the toggle
    for (int i = 0; i < 2; i++)
        block->widgets << new WidgetDef (Widget_RadioButton);

    block->widgets.at(0)->isDefault = false;

    return block;
}

CSVSettings::CustomBlockDef *CSVSettings::WindowPage::buildWindowSize(GroupBlockDef *toggle_def,
                                                                     GroupBlockDef *defined_def,
                                                                     GroupBlockDef *custom_def)
{
    CustomBlockDef *block = new CustomBlockDef(QString ("Window Size"));

    block->blockDefList << toggle_def << defined_def << custom_def;
    block->defaultValue = "Custom";

    return block;

}

void CSVSettings::WindowPage::setupUi()
{
    CustomBlockDef *windowSize = buildWindowSize(buildWindowSizeToggle(),
                                                 buildDefinedWindowSize(),
                                                 buildCustomWindowSize()
                                                 );

    mAbstractBlocks << buildBlock<ToggleBlock> (windowSize);

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


void CSVSettings::WindowPage::initializeWidgets (const CSMSettings::SettingMap &settings)
{
    //iterate each item in each blocks in this section
    //validate the corresponding setting against the defined valuelist if any.
    for (AbstractBlockList::Iterator it_block = mAbstractBlocks.begin();
                                     it_block != mAbstractBlocks.end(); ++it_block)
        (*it_block)->updateSettings (settings);
}
