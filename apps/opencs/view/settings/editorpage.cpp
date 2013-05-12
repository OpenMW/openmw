#include "editorpage.hpp"

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

CSVSettings::EditorPage::EditorPage(QWidget *parent):
    AbstractPage("Editor", parent)
{
    // Hacks to get the stylesheet look properly
#ifdef Q_OS_MAC
    QPlastiqueStyle *style = new QPlastiqueStyle;
    //profilesComboBox->setStyle(style);
#endif

    setupUi();
}

void CSVSettings::EditorPage::setupUi()
{
    GroupBlockDef undoStack (QString("Undo Stack Size"));
    GroupBlockDef topLevelWindowCount (QString("Maximum Top-Level Window Count"));
    GroupBlockDef reuseSubwindow (QString("Reuse Subwindows"));
    GroupBlockDef customWindowSize (QString ("Custom Window Size"));
    GroupBlockDef definedWindowSize (QString ("Pre-Defined Window Size"));
    GroupBlockDef windowSizeToggle (QString ("Window Size"));
    CustomBlockDef windowSize (QString ("Window Size"));

    ////////////////////////////
    //undo stack size property
    ///////////////////////////

    SettingsItemDef *undoStackItem = new SettingsItemDef (undoStack.title, "32");
    undoStack.properties << undoStackItem;
    undoStackItem->minMax.left = "0";
    undoStackItem->minMax.right = "64";

    WidgetDef stackWidget (Widget_SpinBox);
    stackWidget.minMax = &(undoStackItem->minMax);
    stackWidget.widgetWidth = 50;

    undoStackItem->widget = stackWidget;

    //////////////////////////////////////
    //number of top level windows property
    /////////////////////////////////////

    SettingsItemDef *topLevelItem = new SettingsItemDef (topLevelWindowCount.title, "100");
    topLevelWindowCount.properties << topLevelItem;
    topLevelItem->minMax.left = "1";
    topLevelItem->minMax.right = "256";

    WidgetDef topLvlWinWidget (Widget_SpinBox);
    topLvlWinWidget.minMax = &(topLevelItem->minMax);
    topLvlWinWidget.widgetWidth = 50;

    topLevelItem->widget = topLvlWinWidget;

    ///////////////////////////
    //reuse subwindows property
    ////////////////////////////

    SettingsItemDef *reuseSubItem = new SettingsItemDef (reuseSubwindow.title, "Reuse Subwindows");
    *(reuseSubItem->valueList) << "None" << "Top-Level" << "Document-Level";

    WidgetDef reuseSubWidget (Widget_RadioButton);
    reuseSubWidget.valueList = (reuseSubItem->valueList);
    reuseSubWidget.widgetAlignment = Align_Left;

    reuseSubwindow.properties << reuseSubItem;
    reuseSubItem->widget = reuseSubWidget;

    ///////////////////////////////
    //custom window size properties
    ///////////////////////////////

    //custom width
    SettingsItemDef *widthItem = new SettingsItemDef ("Window Width", "640");
    widthItem->widget = WidgetDef (Widget_LineEdit);
    widthItem->widget.widgetWidth = 45;

    //custom height
    SettingsItemDef *heightItem = new SettingsItemDef ("Window Height", "480");
    heightItem->widget = WidgetDef (Widget_LineEdit);
    heightItem->widget.widgetWidth = 45;
    heightItem->widget.caption = "x";

    customWindowSize.properties << widthItem << heightItem;
    customWindowSize.widgetOrientation = Orient_Horizontal;
    customWindowSize.isVisible = false;


    //pre-defined
    SettingsItemDef *widthByHeightItem = new SettingsItemDef ("Window Size", "640x480");
    WidgetDef widthByHeightWidget = WidgetDef (Widget_ComboBox);
    widthByHeightWidget.widgetWidth = 90;
    *(widthByHeightItem->valueList) << "640x480" << "800x600" << "1024x768";

    QStringList *widthProxy = new QStringList;
    QStringList *heightProxy = new QStringList;

    (*widthProxy) << "Window Width" << "640" << "800" << "1024";
    (*heightProxy) << "Window Height" << "480" << "600" << "768";

    *(widthByHeightItem->proxyList) << widthProxy << heightProxy;

    widthByHeightItem->widget = widthByHeightWidget;

    definedWindowSize.properties << widthByHeightItem;
    definedWindowSize.isProxy = true;
    definedWindowSize.isVisible = false;

    // window size toggle
    windowSizeToggle.captions << "Pre-Defined" << "Custom";
    windowSizeToggle.widgetOrientation = Orient_Vertical;
    windowSizeToggle.isVisible = false;

    //define a widget for each group in the toggle
    for (int i = 0; i < 2; i++)
        windowSizeToggle.widgets << new WidgetDef (Widget_RadioButton);

    windowSizeToggle.widgets.at(0)->isDefault = false;

    windowSize.blockDefList << &windowSizeToggle << &definedWindowSize << &customWindowSize;
    windowSize.defaultValue = "Custom";

    QGridLayout *pageLayout = new QGridLayout(this);

    setLayout (pageLayout);

     mAbstractBlocks    << buildBlock<GroupBlock> (topLevelWindowCount)
                        << buildBlock<GroupBlock> (reuseSubwindow)
                        << buildBlock<ToggleBlock> (windowSize)
                        << buildBlock<GroupBlock> (undoStack);

     foreach (AbstractBlock *block, mAbstractBlocks)
     {
         connect (block, SIGNAL (signalUpdateSetting (const QString &, const QString &)),
                  this, SIGNAL (signalUpdateEditorSetting (const QString &, const QString &)) );
     }
}

void CSVSettings::EditorPage::initializeWidgets (const CSMSettings::SettingMap &settings)
{
    //iterate each item in each blocks in this section
    //validate the corresponding setting against the defined valuelist if any.
    for (AbstractBlockList::Iterator it_block = mAbstractBlocks.begin();
                                     it_block != mAbstractBlocks.end(); ++it_block)
        (*it_block)->updateSettings (settings);
}
