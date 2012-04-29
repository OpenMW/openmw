#include "tooltips.hpp"

#include "window_manager.hpp"
#include "widgets.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/world.hpp"
#include "../mwbase/environment.hpp"

#include <boost/lexical_cast.hpp>

using namespace MWGui;
using namespace MyGUI;

ToolTips::ToolTips(WindowManager* windowManager) :
    Layout("openmw_tooltips.xml")
    , mGameMode(true)
    , mWindowManager(windowManager)
    , mFullHelp(false)
{
    getWidget(mDynamicToolTipBox, "DynamicToolTipBox");

    mDynamicToolTipBox->setVisible(false);

    // turn off mouse focus so that getMouseFocusWidget returns the correct widget,
    // even if the mouse is over the tooltip
    mDynamicToolTipBox->setNeedMouseFocus(false);
    mMainWidget->setNeedMouseFocus(false);
}

void ToolTips::onFrame(float frameDuration)
{
    /// \todo Store a MWWorld::Ptr in the widget user data, retrieve it here and construct a tooltip dynamically

    /// \todo we are destroying/creating the tooltip widgets every frame here,
    /// because the tooltip might change (e.g. when trap is activated)
    /// is there maybe a better way (listener when the object changes)?
    for (size_t i=0; i<mDynamicToolTipBox->getChildCount(); ++i)
    {
        mDynamicToolTipBox->_destroyChildWidget(mDynamicToolTipBox->getChildAt(i));
    }

    const IntSize &viewSize = RenderManager::getInstance().getViewSize();

    if (!mGameMode)
    {
        Widget* focus = InputManager::getInstance().getMouseFocusWidget();
        if (focus == 0)
        {
            mDynamicToolTipBox->setVisible(false);
            return;
        }

        IntSize tooltipSize;

        std::string type = focus->getUserString("ToolTipType");
        std::string text = focus->getUserString("ToolTipText");

        ToolTipInfo info;

        if (type == "")
        {
            mDynamicToolTipBox->setVisible(false);
            return;
        }        
        else if (type == "Text")
        {
            info.caption = text;
        }
        else if (type == "CaptionText")
        {
            std::string caption = focus->getUserString("ToolTipCaption");
            info.caption = caption;
            info.text = text;
        }
        else if (type == "ImageCaptionText")
        {
            std::string caption = focus->getUserString("ToolTipCaption");
            std::string image = focus->getUserString("ToolTipImage");
            std::string sizeString = focus->getUserString("ToolTipImageSize");

            info.text = text;
            info.caption = caption;
            info.icon = image;
        }
        tooltipSize = createToolTip(info);

        IntPoint tooltipPosition = InputManager::getInstance().getMousePosition() + IntPoint(0, 24);

        // make the tooltip stay completely in the viewport
        if ((tooltipPosition.left + tooltipSize.width) > viewSize.width)
        {
            tooltipPosition.left = viewSize.width - tooltipSize.width;
        }
        if ((tooltipPosition.top + tooltipSize.height) > viewSize.height)
        {
            tooltipPosition.top = viewSize.height - tooltipSize.height;
        }

        setCoord(tooltipPosition.left, tooltipPosition.top, tooltipSize.width, tooltipSize.height);
        mDynamicToolTipBox->setVisible(true);
    }
    else
    {
        if (!mFocusObject.isEmpty())
        {
            IntSize tooltipSize = getToolTipViaPtr();

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
    // this the maximum width of the tooltip before it starts word-wrapping
    setCoord(0, 0, 300, 300);

    IntSize tooltipSize;

    const MWWorld::Class& object = MWWorld::Class::get (mFocusObject);
    if (!object.hasToolTip(mFocusObject))
    {
        mDynamicToolTipBox->setVisible(false);
    }
    else
    {
        mDynamicToolTipBox->setVisible(true);

        ToolTipInfo info = object.getToolTipInfo(mFocusObject);
        tooltipSize = createToolTip(info);
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

IntSize ToolTips::createToolTip(const ToolTipInfo& info)
{
    std::string caption = info.caption;
    std::string image = info.icon;
    int imageSize = (image != "") ? 32 : 0;
    std::string text = info.text;

    // remove the first newline (easier this way)
    if (text.size() > 0 && text[0] == '\n')
        text.erase(0, 1);

    const ESM::Enchantment* enchant;
    const ESMS::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
    if (info.enchant != "")
    {
        enchant = store.enchants.search(info.enchant);
        if (enchant->data.type == ESM::Enchantment::CastOnce)
            text += "\n" + store.gameSettings.search("sItemCastOnce")->str;
        else if (enchant->data.type == ESM::Enchantment::WhenStrikes)
            text += "\n" + store.gameSettings.search("sItemCastWhenStrikes")->str;
        else if (enchant->data.type == ESM::Enchantment::WhenUsed)
            text += "\n" + store.gameSettings.search("sItemCastWhenUsed")->str;
        else if (enchant->data.type == ESM::Enchantment::ConstantEffect)
            text += "\n" + store.gameSettings.search("sItemCastConstant")->str;
    }

    // this the maximum width of the tooltip before it starts word-wrapping
    setCoord(0, 0, 300, 300);

    const IntPoint padding(8, 8);

    const int imageCaptionHPadding = 8;
    const int imageCaptionVPadding = 4;

    std::string realImage = "icons\\" + image;
    findImageExtension(realImage);

    EditBox* captionWidget = mDynamicToolTipBox->createWidget<EditBox>("NormalText", IntCoord(0, 0, 300, 300), Align::Left | Align::Top, "ToolTipCaption");
    captionWidget->setProperty("Static", "true");
    captionWidget->setCaption(caption);
    IntSize captionSize = captionWidget->getTextSize();

    int captionHeight = std::max(captionSize.height, imageSize);

    EditBox* textWidget = mDynamicToolTipBox->createWidget<EditBox>("SandText", IntCoord(0, captionHeight+imageCaptionVPadding, 300, 300-captionHeight-imageCaptionVPadding), Align::Stretch, "ToolTipText");
    textWidget->setProperty("Static", "true");
    textWidget->setProperty("MultiLine", "true");
    textWidget->setProperty("WordWrap", "true");
    textWidget->setCaption(text);
    textWidget->setTextAlign(Align::HCenter | Align::Top);
    IntSize textSize = textWidget->getTextSize();

    captionSize += IntSize(imageSize, 0); // adjust for image
    IntSize totalSize = IntSize( std::max(textSize.width, captionSize.width + ((image != "") ? imageCaptionHPadding : 0)),
        ((text != "") ? textSize.height + imageCaptionVPadding : 0) + captionHeight );

    if (info.enchant != "")
    {
        Widget* enchantArea = mDynamicToolTipBox->createWidget<Widget>("",
            IntCoord(0, totalSize.height, 300, 300-totalSize.height),
            Align::Stretch, "ToolTipEnchantArea");

        IntCoord coord(0, 6, totalSize.width, 24);

        Widgets::MWEffectListPtr enchantWidget = enchantArea->createWidget<Widgets::MWEffectList>
            ("MW_StatName", coord, Align::Default, "ToolTipEnchantWidget");
        enchantWidget->setWindowManager(mWindowManager);
        enchantWidget->setEnchantmentId(info.enchant);

        std::vector<MyGUI::WidgetPtr> enchantEffectItems;
        enchantWidget->createEffectWidgets(enchantEffectItems, enchantArea, coord, true, (enchant->data.type == ESM::Enchantment::ConstantEffect));
        totalSize.height += coord.top-6;
        totalSize.width = std::max(totalSize.width, coord.width);

        if (enchant->data.type == ESM::Enchantment::WhenStrikes
            || enchant->data.type == ESM::Enchantment::WhenUsed)
        {
            /// \todo store the current enchantment charge somewhere
            int charge = enchant->data.charge;

            const int chargeWidth = 204;

            TextBox* chargeText = enchantArea->createWidget<TextBox>("SandText", IntCoord(0, 0, 10, 18), Align::Default, "ToolTipEnchantChargeText");
            chargeText->setCaption(store.gameSettings.search("sCharges")->str);
            chargeText->setProperty("Static", "true");
            const int chargeTextWidth = chargeText->getTextSize().width + 5;

            const int chargeAndTextWidth = chargeWidth + chargeTextWidth;
            chargeText->setCoord((totalSize.width - chargeAndTextWidth)/2, coord.top+6, chargeTextWidth, 18);

            IntCoord chargeCoord;
            if (totalSize.width < chargeWidth)
            {
                totalSize.width = chargeWidth;
                chargeCoord = IntCoord(0, coord.top+6, chargeWidth, 18);
            }
            else
            {
                chargeCoord = IntCoord((totalSize.width - chargeAndTextWidth)/2 + chargeTextWidth, coord.top+6, chargeWidth, 18);
            }
            Widgets::MWDynamicStatPtr chargeWidget = enchantArea->createWidget<Widgets::MWDynamicStat>
                ("MW_ChargeBar", chargeCoord, Align::Default, "ToolTipEnchantCharge");
            chargeWidget->setValue(charge, charge);
            totalSize.height += 24;
        }
    }

    captionWidget->setCoord( (totalSize.width - captionSize.width)/2 + imageSize,
        (captionHeight-captionSize.height)/2,
        captionSize.width-imageSize,
        captionSize.height);

    captionWidget->setPosition (captionWidget->getPosition() + padding);
    textWidget->setPosition (textWidget->getPosition() + IntPoint(0, padding.top)); // only apply vertical padding, the horizontal works automatically due to Align::HCenter

    if (image != "")
    {
        ImageBox* imageWidget = mDynamicToolTipBox->createWidget<ImageBox>("ImageBox",
            IntCoord((totalSize.width - captionSize.width - imageCaptionHPadding)/2, 0, imageSize, imageSize),
            Align::Left | Align::Top, "ToolTipImage");
        imageWidget->setImageTexture(realImage);
        imageWidget->setPosition (imageWidget->getPosition() + padding);
    }

    totalSize += IntSize(padding.left*2, padding.top*2);

    return totalSize;
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

std::string ToolTips::getValueString(const int value, const std::string& prefix)
{
    if (value == 0)
        return "";
    else
        return "\n" + prefix + ": " + toString(value);
}

std::string ToolTips::getMiscString(const std::string& text, const std::string& prefix)
{
    if (text == "")
        return "";
    else
        return "\n" + prefix + ": " + text;
}

void ToolTips::toggleFullHelp()
{
    mFullHelp = !mFullHelp;
}

bool ToolTips::getFullHelp() const
{
    return mFullHelp;
}
