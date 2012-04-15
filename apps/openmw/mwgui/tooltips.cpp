#include "tooltips.hpp"

using namespace MWGui;
using namespace MyGUI;

ToolTips::ToolTips() :
    Layout("openmw_tooltips.xml")
    , mGameMode(true)
    , mFocusChanged(true)
{
    getWidget(mTextToolTip, "TextToolTip");
    getWidget(mTextToolTipBox, "TextToolTipBox");
    getWidget(mDynamicToolTipBox, "DynamicToolTipBox");

    mDynamicToolTipBox->setVisible(false);

    // turn off mouse focus so that getMouseFocusWidget returns the correct widget,
    // even if the mouse is over the tooltip
    mDynamicToolTipBox->setNeedMouseFocus(false);
    mTextToolTipBox->setNeedMouseFocus(false);
    mTextToolTip->setNeedMouseFocus(false);
    mMainWidget->setNeedMouseFocus(false);
}

void ToolTips::onFrame(float frameDuration)
{
    /// \todo Store a MWWorld::Ptr in the widget user data, retrieve it here and construct a tooltip dynamically

    const IntSize &viewSize = RenderManager::getInstance().getViewSize();

    if (!mGameMode)
    {
        mDynamicToolTipBox->setVisible(false);

        Widget* focus = InputManager::getInstance().getMouseFocusWidget();
        if (focus == 0) return;

        // this the maximum width of the tooltip before it starts word-wrapping
        setCoord(0, 0, 300, 300);

        mTextToolTip->setCaption("Focused: " + focus->getName() + "\nType: " + focus->getTypeName());
        const IntSize &textSize = mTextToolTip->getTextSize();

        IntPoint tooltipPosition = InputManager::getInstance().getMousePosition() + IntPoint(0, 24);

        IntSize size = textSize + IntSize(12, 12);
        // make the tooltip stay completely in the viewport
        if ((tooltipPosition.left + size.width) > viewSize.width)
        {
            tooltipPosition.left = viewSize.width - size.width;
        }
        if ((tooltipPosition.top + size.height) > viewSize.height)
        {
            tooltipPosition.top = viewSize.height - size.height;
        }

        setCoord(tooltipPosition.left, tooltipPosition.top, size.width, size.height);
    }
    else
    {
        mTextToolTipBox->setVisible(false);

        if (!mFocusObject.isEmpty())
        {
            if (mFocusChanged)
            {
                for (size_t i=0; i<mDynamicToolTipBox->getChildCount(); ++i)
                {
                    mDynamicToolTipBox->_destroyChildWidget(mDynamicToolTipBox->getChildAt(i));
                }

                // this the maximum width of the tooltip before it starts word-wrapping
                setCoord(0, 0, 300, 300);

                IntSize tooltipSize;

                /// \todo Not sure about levelled lists (ESM::CreateLevList and ESM::ItemLevList). I think
                /// they are supposed to spawn a concrete object (Creature or item of any type), so
                /// the player wouldn't encounter them and we don't have to handle them here. 

                // --------------------  Door -------------------------------
                if (mFocusObject.getTypeName() == typeid(ESM::Door).name())
                {
                    ESMS::LiveCellRef<ESM::Door, MWWorld::RefData>* ref = mFocusObject.get<ESM::Door>();

                    EditBox* box = mDynamicToolTipBox->createWidget<EditBox>("MW_TextEdit", IntCoord(0, 0, 300, 300), Align::Stretch, "ToolTip");
                    box->setTextAlign(Align::HCenter);
                    box->setProperty("Static", "true");
                    box->setProperty("MultiLine", "true");

                    std::string caption = ref->base->name;
                    /// \todo If destCell is empty, the teleport target is an exterior cell. In that case we 
                    /// need to fetch that cell (via target position) and retrieve the region name.
                    if (ref->ref.teleport && (ref->ref.destCell != ""))
                    {
                        caption += "\n-";
                        caption += "\n"+ref->ref.destCell;
                    }
                    box->setCaption(caption);

                    /// \todo Lock level, trap (retrieve GMST)

                    tooltipSize = box->getTextSize() + IntSize(12,12);

                    mDynamicToolTipBox->setVisible(true);
                }

                // --------------------  NPC -------------------------------
                else if (mFocusObject.getTypeName() == typeid(ESM::NPC).name())
                {
                    /// \todo We don't want tooltips for NPCs in combat mode.
                    ESMS::LiveCellRef<ESM::NPC, MWWorld::RefData>* ref = mFocusObject.get<ESM::NPC>();

                    EditBox* box = mDynamicToolTipBox->createWidget<EditBox>("MW_TextEdit", IntCoord(0, 0, 300, 300), Align::Stretch, "ToolTip");
                    box->setTextAlign(Align::HCenter);
                    box->setProperty("Static", "true");
                    box->setCaption(ref->base->name);

                    tooltipSize = box->getTextSize() + IntSize(12,12);

                    mDynamicToolTipBox->setVisible(true);
                }

                // --------------------  Creature -------------------------------
                else if (mFocusObject.getTypeName() == typeid(ESM::Creature).name())
                {
                    ESMS::LiveCellRef<ESM::Creature, MWWorld::RefData>* ref = mFocusObject.get<ESM::Creature>();

                    EditBox* box = mDynamicToolTipBox->createWidget<EditBox>("MW_TextEdit", IntCoord(0, 0, 300, 300), Align::Stretch, "ToolTip");
                    box->setTextAlign(Align::HCenter);
                    box->setProperty("Static", "true");
                    box->setCaption(ref->base->name);

                    /// \todo

                    tooltipSize = box->getTextSize() + IntSize(12,12);

                    mDynamicToolTipBox->setVisible(true);
                }

                // --------------------  CreatureLevList -------------------------------
                else if (mFocusObject.getTypeName() == typeid(ESM::Creature).name())
                {
                    ESMS::LiveCellRef<ESM::Creature, MWWorld::RefData>* ref = mFocusObject.get<ESM::Creature>();

                    EditBox* box = mDynamicToolTipBox->createWidget<EditBox>("MW_TextEdit", IntCoord(0, 0, 300, 300), Align::Stretch, "ToolTip");
                    box->setTextAlign(Align::HCenter);
                    box->setProperty("Static", "true");
                    box->setCaption(ref->base->name);

                    /// \todo

                    tooltipSize = box->getTextSize() + IntSize(12,12);

                    mDynamicToolTipBox->setVisible(true);
                }

                // --------------------  Container -------------------------------
                else if (mFocusObject.getTypeName() == typeid(ESM::Container).name())
                {
                    ESMS::LiveCellRef<ESM::Container, MWWorld::RefData>* ref = mFocusObject.get<ESM::Container>();

                    EditBox* box = mDynamicToolTipBox->createWidget<EditBox>("MW_TextEdit", IntCoord(0, 0, 300, 300), Align::Stretch, "ToolTip");
                    box->setTextAlign(Align::HCenter);
                    box->setProperty("Static", "true");
                    box->setCaption(ref->base->name);

                    /// \todo Lock level, trap (retrieve GMST)

                    tooltipSize = box->getTextSize() + IntSize(12,12);

                    mDynamicToolTipBox->setVisible(true);
                }

                // --------------------  Potion -------------------------------
                else if (mFocusObject.getTypeName() == typeid(ESM::Potion).name())
                {
                    ESMS::LiveCellRef<ESM::Potion, MWWorld::RefData>* ref = mFocusObject.get<ESM::Potion>();

                    EditBox* box = mDynamicToolTipBox->createWidget<EditBox>("MW_TextEdit", IntCoord(0, 0, 300, 300), Align::Stretch, "ToolTip");
                    box->setTextAlign(Align::HCenter);
                    box->setProperty("Static", "true");
                    box->setCaption(ref->base->name);

                    /// \todo

                    tooltipSize = box->getTextSize() + IntSize(12,12);

                    mDynamicToolTipBox->setVisible(true);
                }

                // --------------------  Apparatus -------------------------------
                else if (mFocusObject.getTypeName() == typeid(ESM::Apparatus).name())
                {
                    ESMS::LiveCellRef<ESM::Potion, MWWorld::RefData>* ref = mFocusObject.get<ESM::Potion>();

                    EditBox* box = mDynamicToolTipBox->createWidget<EditBox>("MW_TextEdit", IntCoord(0, 0, 300, 300), Align::Stretch, "ToolTip");
                    box->setTextAlign(Align::HCenter);
                    box->setProperty("Static", "true");
                    box->setCaption(ref->base->name);

                    /// \todo

                    tooltipSize = box->getTextSize() + IntSize(12,12);

                    mDynamicToolTipBox->setVisible(true);
                }

                // --------------------  Armor -------------------------------
                else if (mFocusObject.getTypeName() == typeid(ESM::Armor).name())
                {
                    ESMS::LiveCellRef<ESM::Armor, MWWorld::RefData>* ref = mFocusObject.get<ESM::Armor>();

                    EditBox* box = mDynamicToolTipBox->createWidget<EditBox>("MW_TextEdit", IntCoord(0, 0, 300, 300), Align::Stretch, "ToolTip");
                    box->setTextAlign(Align::HCenter);
                    box->setProperty("Static", "true");
                    box->setCaption(ref->base->name);

                    /// \todo weight, armor value, value, durability..

                    tooltipSize = box->getTextSize() + IntSize(12,12);

                    mDynamicToolTipBox->setVisible(true);
                }

                // --------------------  Book -------------------------------
                else if (mFocusObject.getTypeName() == typeid(ESM::Book).name())
                {
                    ESMS::LiveCellRef<ESM::Book, MWWorld::RefData>* ref = mFocusObject.get<ESM::Book>();

                    EditBox* box = mDynamicToolTipBox->createWidget<EditBox>("MW_TextEdit", IntCoord(0, 0, 300, 300), Align::Stretch, "ToolTip");
                    box->setTextAlign(Align::HCenter);
                    box->setProperty("Static", "true");
                    box->setCaption(ref->base->name);

                    /// \todo

                    tooltipSize = box->getTextSize() + IntSize(12,12);

                    mDynamicToolTipBox->setVisible(true);
                }

                // --------------------  Clothing -------------------------------
                else if (mFocusObject.getTypeName() == typeid(ESM::Clothing).name())
                {
                    ESMS::LiveCellRef<ESM::Clothing, MWWorld::RefData>* ref = mFocusObject.get<ESM::Clothing>();

                    EditBox* box = mDynamicToolTipBox->createWidget<EditBox>("MW_TextEdit", IntCoord(0, 0, 300, 300), Align::Stretch, "ToolTip");
                    box->setTextAlign(Align::HCenter);
                    box->setProperty("Static", "true");
                    box->setCaption(ref->base->name);

                    /// \todo

                    tooltipSize = box->getTextSize() + IntSize(12,12);

                    mDynamicToolTipBox->setVisible(true);
                }

                // --------------------  Ingredient -------------------------------
                else if (mFocusObject.getTypeName() == typeid(ESM::Ingredient).name())
                {
                    ESMS::LiveCellRef<ESM::Ingredient, MWWorld::RefData>* ref = mFocusObject.get<ESM::Ingredient>();

                    EditBox* box = mDynamicToolTipBox->createWidget<EditBox>("MW_TextEdit", IntCoord(0, 0, 300, 300), Align::Stretch, "ToolTip");
                    box->setTextAlign(Align::HCenter);
                    box->setProperty("Static", "true");
                    box->setCaption(ref->base->name);

                    /// \todo

                    tooltipSize = box->getTextSize() + IntSize(12,12);

                    mDynamicToolTipBox->setVisible(true);
                }

                // --------------------  Light -------------------------------
                else if (mFocusObject.getTypeName() == typeid(ESM::Light).name())
                {
                    ESMS::LiveCellRef<ESM::Light, MWWorld::RefData>* ref = mFocusObject.get<ESM::Light>();

                    EditBox* box = mDynamicToolTipBox->createWidget<EditBox>("MW_TextEdit", IntCoord(0, 0, 300, 300), Align::Stretch, "ToolTip");
                    box->setTextAlign(Align::HCenter);
                    box->setProperty("Static", "true");
                    box->setCaption(ref->base->name);

                    /// \todo

                    tooltipSize = box->getTextSize() + IntSize(12,12);

                    mDynamicToolTipBox->setVisible(ref->base->name != "");
                }

                // --------------------  Tool -------------------------------
                else if (mFocusObject.getTypeName() == typeid(ESM::Tool).name())
                {
                    ESMS::LiveCellRef<ESM::Tool, MWWorld::RefData>* ref = mFocusObject.get<ESM::Tool>();

                    EditBox* box = mDynamicToolTipBox->createWidget<EditBox>("MW_TextEdit", IntCoord(0, 0, 300, 300), Align::Stretch, "ToolTip");
                    box->setTextAlign(Align::HCenter);
                    box->setProperty("Static", "true");
                    box->setCaption(ref->base->name);

                    /// \todo

                    tooltipSize = box->getTextSize() + IntSize(12,12);

                    mDynamicToolTipBox->setVisible(true);
                }

                // --------------------  Miscellaneous -------------------------------
                else if (mFocusObject.getTypeName() == typeid(ESM::Miscellaneous).name())
                {
                    ESMS::LiveCellRef<ESM::Miscellaneous, MWWorld::RefData>* ref = mFocusObject.get<ESM::Miscellaneous>();

                    EditBox* box = mDynamicToolTipBox->createWidget<EditBox>("MW_TextEdit", IntCoord(0, 0, 300, 300), Align::Stretch, "ToolTip");
                    box->setTextAlign(Align::HCenter);
                    box->setProperty("Static", "true");
                    box->setCaption(ref->base->name);

                    /// \todo

                    tooltipSize = box->getTextSize() + IntSize(12,12);

                    mDynamicToolTipBox->setVisible(true);
                }

                // --------------------  Probe -------------------------------
                else if (mFocusObject.getTypeName() == typeid(ESM::Probe).name())
                {
                    ESMS::LiveCellRef<ESM::Probe, MWWorld::RefData>* ref = mFocusObject.get<ESM::Probe>();

                    EditBox* box = mDynamicToolTipBox->createWidget<EditBox>("MW_TextEdit", IntCoord(0, 0, 300, 300), Align::Stretch, "ToolTip");
                    box->setTextAlign(Align::HCenter);
                    box->setProperty("Static", "true");
                    box->setCaption(ref->base->name);

                    /// \todo

                    tooltipSize = box->getTextSize() + IntSize(12,12);

                    mDynamicToolTipBox->setVisible(true);
                }

                // --------------------  Repair -------------------------------
                else if (mFocusObject.getTypeName() == typeid(ESM::Repair).name())
                {
                    ESMS::LiveCellRef<ESM::Repair, MWWorld::RefData>* ref = mFocusObject.get<ESM::Repair>();

                    EditBox* box = mDynamicToolTipBox->createWidget<EditBox>("MW_TextEdit", IntCoord(0, 0, 300, 300), Align::Stretch, "ToolTip");
                    box->setTextAlign(Align::HCenter);
                    box->setProperty("Static", "true");
                    box->setCaption(ref->base->name);

                    /// \todo

                    tooltipSize = box->getTextSize() + IntSize(12,12);

                    mDynamicToolTipBox->setVisible(true);
                }

                // --------------------  Weapon -------------------------------
                else if (mFocusObject.getTypeName() == typeid(ESM::Weapon).name())
                {
                    ESMS::LiveCellRef<ESM::Weapon, MWWorld::RefData>* ref = mFocusObject.get<ESM::Weapon>();

                    EditBox* box = mDynamicToolTipBox->createWidget<EditBox>("MW_TextEdit", IntCoord(0, 0, 300, 300), Align::Stretch, "ToolTip");
                    box->setTextAlign(Align::HCenter);
                    box->setProperty("Static", "true");
                    box->setCaption(ref->base->name);

                    /// \todo

                    tooltipSize = box->getTextSize() + IntSize(12,12);

                    mDynamicToolTipBox->setVisible(true);
                }

                // --------------------  Activator -------------------------------
                else if (mFocusObject.getTypeName() == typeid(ESM::Activator).name())
                {
                    ESMS::LiveCellRef<ESM::Activator, MWWorld::RefData>* ref = mFocusObject.get<ESM::Activator>();

                    EditBox* box = mDynamicToolTipBox->createWidget<EditBox>("MW_TextEdit", IntCoord(0, 0, 300, 300), Align::Stretch, "ToolTip");
                    box->setTextAlign(Align::HCenter);
                    box->setProperty("Static", "true");
                    box->setCaption(ref->base->name);

                    /// \todo

                    tooltipSize = box->getTextSize() + IntSize(12,12);

                    mDynamicToolTipBox->setVisible(ref->base->name != "");
                }

                else
                {
                    // object without tooltip
                    mDynamicToolTipBox->setVisible(false);
                }

                // adjust tooltip size to fit its content, position it above the crosshair
                /// \todo Slide the tooltip along the bounding box of the focused object (like in Morrowind)
                setCoord(viewSize.width/2 - (tooltipSize.width)/2.f,
                        viewSize.height/2 - (tooltipSize.height) - 32,
                        tooltipSize.width,
                        tooltipSize.height);
            }
            mFocusChanged = false;
        }
        else
            mDynamicToolTipBox->setVisible(false);
    }
}

void ToolTips::enterGameMode()
{
    mGameMode = true;
}

void ToolTips::enterGuiMode()
{
    mGameMode = false;
}

void ToolTips::setFocusObject(const MWWorld::Ptr& focus)
{
    if (focus != mFocusObject)
    {
        mFocusObject = focus;
        mFocusChanged = true;
    }
}
