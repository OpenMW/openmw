#include "tooltips.hpp"
#include "window_manager.hpp"

#include <boost/lexical_cast.hpp>

using namespace MWGui;
using namespace MyGUI;

ToolTips::ToolTips(WindowManager* windowManager) :
    Layout("openmw_tooltips.xml")
    , mGameMode(true)
    , mWindowManager(windowManager)
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
        mTextToolTipBox->setVisible(true);

        Widget* focus = InputManager::getInstance().getMouseFocusWidget();
        if (focus == 0)
        {
            mTextToolTipBox->setVisible(false);
            return;
        }

        std::string type = focus->getUserString("ToolTipType");
        std::string text = focus->getUserString("ToolTipText");
        if (type == "" || text == "")
        {
            mTextToolTipBox->setVisible(false);
            return;
        }

        // this the maximum width of the tooltip before it starts word-wrapping
        setCoord(0, 0, 300, 300);

        mTextToolTip->setCaption(text);
        const IntSize &textSize = mTextToolTip->getTextSize();

        IntPoint tooltipPosition = InputManager::getInstance().getMousePosition() + IntPoint(0, 24);

        IntSize size = textSize + IntSize(6, 6);
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
            IntSize tooltipSize = getToolTipViaPtr();

            tooltipSize += IntSize(6,6); // padding, adjust for skin

            // adjust tooltip size to fit its content, position it above the crosshair
            /// \todo Slide the tooltip along the bounding box of the focused object (like in Morrowind)
            setCoord(viewSize.width/2 - (tooltipSize.width)/2.f,
                    viewSize.height/2 - (tooltipSize.height) - 32,
                    tooltipSize.width,
                    tooltipSize.height);
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
    mFocusObject = focus;
}

IntSize ToolTips::getToolTipViaPtr ()
{
    /// \todo we are destroying/creating the tooltip widgets every frame here,
    /// because the tooltip might change (e.g. when trap is activated)
    /// is there maybe a better way (listener when the object changes)?
    for (size_t i=0; i<mDynamicToolTipBox->getChildCount(); ++i)
    {
        mDynamicToolTipBox->_destroyChildWidget(mDynamicToolTipBox->getChildAt(i));
    }

    // this the maximum width of the tooltip before it starts word-wrapping
    setCoord(0, 0, 300, 300);

    IntSize tooltipSize;

    // --------------------  Door -------------------------------
    if (mFocusObject.getTypeName() == typeid(ESM::Door).name())
    {
        ESMS::LiveCellRef<ESM::Door, MWWorld::RefData>* ref = mFocusObject.get<ESM::Door>();

        std::string text;
        /// \todo If destCell is empty, the teleport target is an exterior cell. In that case we 
        /// need to fetch that cell (via target position) and retrieve the region name.
        if (ref->ref.teleport && (ref->ref.destCell != ""))
        {
            text += "\n" + mWindowManager->getGameSettingString("sTo", "to");
            text += "\n"+ref->ref.destCell;
        }

        if (ref->ref.lockLevel > 0)
            text += "\n" + mWindowManager->getGameSettingString("sLockLevel", "Lock") + ": " + toString(ref->ref.lockLevel);
        if (ref->ref.trap != "")
            text += "\n" + mWindowManager->getGameSettingString("sTrapped", "Trapped!");

        tooltipSize = createToolTip(ref->base->name, text);
    }

    // --------------------  NPC -------------------------------
    else if (mFocusObject.getTypeName() == typeid(ESM::NPC).name())
    {
        /// \todo We don't want tooltips for NPCs in combat mode.
        ESMS::LiveCellRef<ESM::NPC, MWWorld::RefData>* ref = mFocusObject.get<ESM::NPC>();

        tooltipSize = createToolTip(ref->base->name, "");
    }

    // --------------------  Creature -------------------------------
    else if (mFocusObject.getTypeName() == typeid(ESM::Creature).name())
    {
        /// \todo We don't want tooltips for Creatures in combat mode.
        ESMS::LiveCellRef<ESM::Creature, MWWorld::RefData>* ref = mFocusObject.get<ESM::Creature>();

        tooltipSize = createToolTip(ref->base->name, "");
    }

    // --------------------  Container -------------------------------
    else if (mFocusObject.getTypeName() == typeid(ESM::Container).name())
    {
        ESMS::LiveCellRef<ESM::Container, MWWorld::RefData>* ref = mFocusObject.get<ESM::Container>();

        std::string text;

        if (ref->ref.lockLevel > 0)
            text += "\n" + mWindowManager->getGameSettingString("sLockLevel", "Lock") + ": " + toString(ref->ref.lockLevel);
        if (ref->ref.trap != "")
            text += "\n" + mWindowManager->getGameSettingString("sTrapped", "Trapped!");

        tooltipSize = createToolTip(ref->base->name, text);
    }

    // --------------------  Potion -------------------------------
    else if (mFocusObject.getTypeName() == typeid(ESM::Potion).name())
    {
        ESMS::LiveCellRef<ESM::Potion, MWWorld::RefData>* ref = mFocusObject.get<ESM::Potion>();

        /// \todo magic effects
        std::string text;
        text += "\n" + mWindowManager->getGameSettingString("sWeight", "Weight") + ": " + toString(ref->base->data.weight);
        text += getValueString(ref->base->data.value);

        tooltipSize = createImageToolTip(ref->base->name, ref->base->icon, text);
    }

    // --------------------  Apparatus -------------------------------
    else if (mFocusObject.getTypeName() == typeid(ESM::Apparatus).name())
    {
        ESMS::LiveCellRef<ESM::Apparatus, MWWorld::RefData>* ref = mFocusObject.get<ESM::Apparatus>();

        std::string text;
        text += "\n" + mWindowManager->getGameSettingString("sQuality", "Quality") + ": " + toString(ref->base->data.quality);
        text += "\n" + mWindowManager->getGameSettingString("sWeight", "Weight") + ": " + toString(ref->base->data.weight);
        text += getValueString(ref->base->data.value);

        tooltipSize = createImageToolTip(ref->base->name, ref->base->icon, text);
    }

    // --------------------  Armor -------------------------------
    else if (mFocusObject.getTypeName() == typeid(ESM::Armor).name())
    {
        ESMS::LiveCellRef<ESM::Armor, MWWorld::RefData>* ref = mFocusObject.get<ESM::Armor>();

        /// \todo magic effects, armor type (medium/light/heavy)
        std::string text;
        text += "\n" + mWindowManager->getGameSettingString("sArmorRating", "Armor Rating") + ": " + toString(ref->base->data.armor);

        /// \todo where is the current armor health stored?
        //text += "\n" + mWindowManager->getGameSettingString("sCondition", "Condition") + ": " + toString(ref->base->data.health);
        text += "\n" + mWindowManager->getGameSettingString("sWeight", "Weight") + ": " + toString(ref->base->data.weight);
        text += getValueString(ref->base->data.value);

        tooltipSize = createImageToolTip(ref->base->name, ref->base->icon, text);
    }

    // --------------------  Book -------------------------------
    else if (mFocusObject.getTypeName() == typeid(ESM::Book).name())
    {
        ESMS::LiveCellRef<ESM::Book, MWWorld::RefData>* ref = mFocusObject.get<ESM::Book>();

        std::string text;
        text += "\n" + mWindowManager->getGameSettingString("sWeight", "Weight") + ": " + toString(ref->base->data.weight);
        text += getValueString(ref->base->data.value);

        tooltipSize = createImageToolTip(ref->base->name, ref->base->icon, text);
    }

    // --------------------  Clothing -------------------------------
    else if (mFocusObject.getTypeName() == typeid(ESM::Clothing).name())
    {
        ESMS::LiveCellRef<ESM::Clothing, MWWorld::RefData>* ref = mFocusObject.get<ESM::Clothing>();

        /// \todo magic effects
        std::string text;
        text += "\n" + mWindowManager->getGameSettingString("sWeight", "Weight") + ": " + toString(ref->base->data.weight);
        text += getValueString(ref->base->data.value);

        tooltipSize = createImageToolTip(ref->base->name, ref->base->icon, text);
    }

    // --------------------  Ingredient -------------------------------
    else if (mFocusObject.getTypeName() == typeid(ESM::Ingredient).name())
    {
        ESMS::LiveCellRef<ESM::Ingredient, MWWorld::RefData>* ref = mFocusObject.get<ESM::Ingredient>();

        /// \todo magic effects
        std::string text;
        text += "\n" + mWindowManager->getGameSettingString("sWeight", "Weight") + ": " + toString(ref->base->data.weight);
        text += getValueString(ref->base->data.value);

        tooltipSize = createImageToolTip(ref->base->name, ref->base->icon, text);
    }

    // --------------------  Light -------------------------------
    else if (mFocusObject.getTypeName() == typeid(ESM::Light).name())
    {
        ESMS::LiveCellRef<ESM::Light, MWWorld::RefData>* ref = mFocusObject.get<ESM::Light>();

        std::string text;
        text += "\n" + mWindowManager->getGameSettingString("sWeight", "Weight") + ": " + toString(ref->base->data.weight);
        text += getValueString(ref->base->data.value);

        tooltipSize = createImageToolTip(ref->base->name, ref->base->icon, text);
    }

    // --------------------  Tool -------------------------------
    else if (mFocusObject.getTypeName() == typeid(ESM::Tool).name())
    {
        ESMS::LiveCellRef<ESM::Tool, MWWorld::RefData>* ref = mFocusObject.get<ESM::Tool>();

        std::string text;
        text += "\n" + mWindowManager->getGameSettingString("sUses", "Uses") + ": " + toString(ref->base->data.uses);
        text += "\n" + mWindowManager->getGameSettingString("sQuality", "Quality") + ": " + toString(ref->base->data.quality);
        text += "\n" + mWindowManager->getGameSettingString("sWeight", "Weight") + ": " + toString(ref->base->data.weight);
        text += getValueString(ref->base->data.value);

        tooltipSize = createImageToolTip(ref->base->name, ref->base->icon, text);
    }

    // --------------------  Miscellaneous -------------------------------
    else if (mFocusObject.getTypeName() == typeid(ESM::Miscellaneous).name())
    {
        ESMS::LiveCellRef<ESM::Miscellaneous, MWWorld::RefData>* ref = mFocusObject.get<ESM::Miscellaneous>();

        std::string text;
        text += "\n" + mWindowManager->getGameSettingString("sWeight", "Weight") + ": " + toString(ref->base->data.weight);
        text += getValueString(ref->base->data.value);

        tooltipSize = createImageToolTip(ref->base->name, ref->base->icon, text);
    }

    // --------------------  Probe -------------------------------
    else if (mFocusObject.getTypeName() == typeid(ESM::Probe).name())
    {
        ESMS::LiveCellRef<ESM::Probe, MWWorld::RefData>* ref = mFocusObject.get<ESM::Probe>();

        std::string text;
        text += "\n" + mWindowManager->getGameSettingString("sUses", "Uses") + ": " + toString(ref->base->data.uses);
        text += "\n" + mWindowManager->getGameSettingString("sQuality", "Quality") + ": " + toString(ref->base->data.quality);
        text += "\n" + mWindowManager->getGameSettingString("sWeight", "Weight") + ": " + toString(ref->base->data.weight);
        text += getValueString(ref->base->data.value);

        tooltipSize = createImageToolTip(ref->base->name, ref->base->icon, text);
    }

    // --------------------  Repair -------------------------------
    else if (mFocusObject.getTypeName() == typeid(ESM::Repair).name())
    {
        ESMS::LiveCellRef<ESM::Repair, MWWorld::RefData>* ref = mFocusObject.get<ESM::Repair>();

        std::string text;
        text += "\n" + mWindowManager->getGameSettingString("sUses", "Uses") + ": " + toString(ref->base->data.uses);
        text += "\n" + mWindowManager->getGameSettingString("sQuality", "Quality") + ": " + toString(ref->base->data.quality);
        text += "\n" + mWindowManager->getGameSettingString("sWeight", "Weight") + ": " + toString(ref->base->data.weight);
        text += getValueString(ref->base->data.value);

        tooltipSize = createImageToolTip(ref->base->name, ref->base->icon, text);
    }

    // --------------------  Weapon -------------------------------
    else if (mFocusObject.getTypeName() == typeid(ESM::Weapon).name())
    {
        ESMS::LiveCellRef<ESM::Weapon, MWWorld::RefData>* ref = mFocusObject.get<ESM::Weapon>();

        /// \todo weapon damage, magic effects, health (condition)

        std::string text;
        text += "\n" + mWindowManager->getGameSettingString("sWeight", "Weight") + ": " + toString(ref->base->data.weight);
        text += getValueString(ref->base->data.value);

        tooltipSize = createImageToolTip(ref->base->name, ref->base->icon, "");
    }

    // --------------------  Activator -------------------------------
    else if (mFocusObject.getTypeName() == typeid(ESM::Activator).name())
    {
        ESMS::LiveCellRef<ESM::Activator, MWWorld::RefData>* ref = mFocusObject.get<ESM::Activator>();

        tooltipSize = createToolTip(ref->base->name, "");
    }

    else
    {
        // object without tooltip
        mDynamicToolTipBox->setVisible(false);
    }

    return tooltipSize;
}

void ToolTips::findImageExtension(std::string& image)
{
    int len = image.size();
    if (len < 4) return;

    if (!Ogre::ResourceGroupManager::getSingleton().resourceExists(Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME, image))
    {
        // Change texture extension to .dds
        image[len-3] = 'd';
        image[len-2] = 'd';
        image[len-1] = 's';
    }
}

IntSize ToolTips::createImageToolTip(const std::string& caption, const std::string& image, const std::string& text)
{
    // remove the first newline (easier this way)
    std::string realText = text;
    if (realText.size() > 0)
        realText.erase(0, 1);

    std::string realImage = "icons\\" + image;
    findImageExtension(realImage);

    const int imageSize = 32;

    EditBox* captionWidget = mDynamicToolTipBox->createWidget<EditBox>("NormalText", IntCoord(0, 0, 300, 300), Align::Left | Align::Top, "ToolTipCaption");
    captionWidget->setProperty("Static", "true");
    captionWidget->setCaption(caption);
    EditBox* textWidget = mDynamicToolTipBox->createWidget<EditBox>("SandText", IntCoord(0, imageSize, 300, 262), Align::Stretch, "ToolTipText");
    textWidget->setProperty("Static", "true");
    textWidget->setProperty("MultiLine", "true");
    textWidget->setCaption(realText);
    textWidget->setTextAlign(Align::HCenter);

    IntSize captionSize = captionWidget->getTextSize();
    IntSize textSize = textWidget->getTextSize();

    captionSize += IntSize(imageSize, 0); // adjust for image
    IntSize totalSize = IntSize( std::max(textSize.width, captionSize.width), ((realText != "") ? textSize.height : 0) + imageSize );

    ImageBox* imageWidget = mDynamicToolTipBox->createWidget<ImageBox>("ImageBox",
        IntCoord((totalSize.width - captionSize.width)/2, 0, imageSize, imageSize),
        Align::Left | Align::Top, "ToolTipImage");
    imageWidget->setImageTexture(realImage);

    captionWidget->setCoord( (totalSize.width - captionSize.width)/2 + imageSize, (32-captionSize.height)/2, captionSize.width-imageSize, captionSize.height);

    mDynamicToolTipBox->setVisible(caption != "");

    return totalSize;
}

IntSize ToolTips::createToolTip(const std::string& caption, const std::string& text)
{
    // remove the first newline (easier this way)
    std::string realText = text;
    if (realText.size() > 0)
        realText.erase(0, 1);

    EditBox* box = mDynamicToolTipBox->createWidget<EditBox>("NormalText", IntCoord(0, 0, 300, 300), Align::Stretch, "ToolTip");
    box->setTextAlign(Align::HCenter);
    box->setProperty("Static", "true");
    box->setProperty("MultiLine", "true");
    box->setCaption(caption + (realText != "" ? "\n#BF9959" + realText : ""));

    mDynamicToolTipBox->setVisible(caption != "");

    return box->getTextSize();
}

std::string ToolTips::toString(const float value)
{
    std::ostringstream stream;
    stream << std::setprecision(3) << value;
    return stream.str();
}

std::string ToolTips::toString(const int value)
{
    std::ostringstream stream;
    stream << value;
    return stream.str();
}

std::string ToolTips::getValueString(const int value)
{
    if (value == 0)
        return "";
    else
        return "\n" + mWindowManager->getGameSettingString("sValue", "Value") + ": " + toString(value);
}
