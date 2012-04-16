#include "tooltips.hpp"
#include "window_manager.hpp"

#include "../mwworld/class.hpp"

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

        // this the maximum width of the tooltip before it starts word-wrapping
        setCoord(0, 0, 300, 300);

        IntSize tooltipSize;

        std::string type = focus->getUserString("ToolTipType");
        std::string text = focus->getUserString("ToolTipText");
        if (type == "")
        {
            mDynamicToolTipBox->setVisible(false);
            return;
        }
        else if (type == "Text")
            tooltipSize = createToolTip(text);
        else if (type == "CaptionText")
        {
            std::string caption = focus->getUserString("ToolTipCaption");
            tooltipSize = createToolTip(caption, text);
        }
        else if (type == "ImageCaptionText")
        {
            std::string caption = focus->getUserString("ToolTipCaption");
            std::string image = focus->getUserString("ToolTipImage");
            std::string sizeString = focus->getUserString("ToolTipImageSize");
            int size = (sizeString != "" ? boost::lexical_cast<int>(sizeString) : 32);
            tooltipSize = createImageToolTip(caption, image, size, text);
        }

        IntPoint tooltipPosition = InputManager::getInstance().getMousePosition() + IntPoint(0, 24);

        IntSize size = tooltipSize + IntSize(6, 6);
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

        ToolTipInfo info = object.getToolTipInfo(mFocusObject, mWindowManager->getEnvironment());
        if (info.icon == "")
        {
            tooltipSize= createToolTip(info.caption, info.text);
        }
        else
        {
            tooltipSize = createImageToolTip(info.caption, info.icon, 32, info.text);
        }
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

IntSize ToolTips::createImageToolTip(const std::string& caption, const std::string& image, const int imageSize, const std::string& text)
{
    // remove the first newline (easier this way)
    std::string realText = text;
    if (realText.size() > 0 && realText[0] == '\n')
        realText.erase(0, 1);

    std::string realImage = "icons\\" + image;
    findImageExtension(realImage);

    EditBox* captionWidget = mDynamicToolTipBox->createWidget<EditBox>("NormalText", IntCoord(0, 0, 300, 300), Align::Left | Align::Top, "ToolTipCaption");
    captionWidget->setProperty("Static", "true");
    captionWidget->setCaption(caption);
    EditBox* textWidget = mDynamicToolTipBox->createWidget<EditBox>("SandText", IntCoord(0, imageSize, 300, 300-imageSize), Align::Stretch, "ToolTipText");
    textWidget->setProperty("Static", "true");
    textWidget->setProperty("MultiLine", "true");
    textWidget->setProperty("WordWrap", "true");
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

    captionWidget->setCoord( (totalSize.width - captionSize.width)/2 + imageSize, (imageSize-captionSize.height)/2, captionSize.width-imageSize, captionSize.height);

    return totalSize;
}

IntSize ToolTips::createToolTip(const std::string& caption, const std::string& text)
{
    // remove the first newline (easier this way)
    std::string realText = text;
    if (realText.size() > 0 && realText[0] == '\n')
        realText.erase(0, 1);

    EditBox* box = mDynamicToolTipBox->createWidget<EditBox>("NormalText", IntCoord(0, 0, 300, 300), Align::Stretch, "ToolTip");
    box->setTextAlign(Align::HCenter);
    box->setProperty("Static", "true");
    box->setProperty("MultiLine", "true");
    box->setProperty("WordWrap", "true");
    box->setCaption(caption + (realText != "" ? "\n#BF9959" + realText : ""));

    return box->getTextSize();
}

IntSize ToolTips::createToolTip(const std::string& text)
{
    EditBox* box = mDynamicToolTipBox->createWidget<EditBox>("SandText", IntCoord(0, 0, 300, 300), Align::Stretch, "ToolTip");
    box->setTextAlign(Align::HCenter);
    box->setProperty("Static", "true");
    box->setProperty("MultiLine", "true");
    box->setProperty("WordWrap", "true");
    box->setCaption(text);

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
