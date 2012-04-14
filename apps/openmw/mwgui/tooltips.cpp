#include "tooltips.hpp"

using namespace MWGui;
using namespace MyGUI;

ToolTips::ToolTips() :
    Layout("openmw_tooltips.xml")
    , mGameMode(true)
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

void ToolTips::enterGameMode()
{
    mGameMode = true;
}

void ToolTips::enterGuiMode()
{
    mGameMode = false;
}
