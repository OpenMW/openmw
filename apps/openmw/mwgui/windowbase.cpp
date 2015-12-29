#include "windowbase.hpp"

#include <MyGUI_InputManager.h>
#include <MyGUI_RenderManager.h>

#include <components/settings/settings.hpp>

#include "../mwbase/windowmanager.hpp"
#include "../mwbase/environment.hpp"

#include "draganddrop.hpp"

using namespace MWGui;

WindowBase::WindowBase(const std::string& parLayout)
  : Layout(parLayout)
{
}

void WindowBase::setVisible(bool visible)
{
    bool wasVisible = mMainWidget->getVisible();
    mMainWidget->setVisible(visible);

    if (visible)
        open();
    else if (wasVisible && !visible)
        close();

    // This is needed as invisible widgets can retain key focus.
    // Remove for MyGUI 3.2.2
    if (!visible)
    {
        MyGUI::Widget* keyFocus = MyGUI::InputManager::getInstance().getKeyFocusWidget();
        while (keyFocus != mMainWidget && keyFocus != NULL)
            keyFocus = keyFocus->getParent();

        if (keyFocus == mMainWidget)
            MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(NULL);
    }
}

bool WindowBase::isVisible()
{
    return mMainWidget->getVisible();
}

void WindowBase::center()
{
    // Centre dialog

    MyGUI::IntSize layerSize = MyGUI::RenderManager::getInstance().getViewSize();
    if (mMainWidget->getLayer())
        layerSize = mMainWidget->getLayer()->getSize();

    MyGUI::IntCoord coord = mMainWidget->getCoord();
    coord.left = (layerSize.width - coord.width)/2;
    coord.top = (layerSize.height - coord.height)/2;
    mMainWidget->setCoord(coord);
}

WindowModal::WindowModal(const std::string& parLayout)
    : WindowBase(parLayout)
{
}

void WindowModal::open()
{
    MyGUI::InputManager::getInstance ().addWidgetModal (mMainWidget);
    MWBase::Environment::get().getWindowManager()->addCurrentModal(this); //Set so we can escape it if needed
}

void WindowModal::close()
{
    MyGUI::InputManager::getInstance ().removeWidgetModal (mMainWidget);
    MWBase::Environment::get().getWindowManager()->removeCurrentModal(this);
}

NoDrop::NoDrop(DragAndDrop *drag, MyGUI::Widget *widget)
    : mWidget(widget), mDrag(drag), mTransparent(false)
{
    if (!mWidget)
        throw std::runtime_error("NoDrop needs a non-NULL widget!");
}

void NoDrop::onFrame(float dt)
{
    MyGUI::IntPoint mousePos = MyGUI::InputManager::getInstance().getMousePosition();

    if (mDrag->mIsOnDragAndDrop)
    {
        MyGUI::Widget* focus = MyGUI::InputManager::getInstance().getMouseFocusWidget();
        while (focus && focus != mWidget)
            focus = focus->getParent();

        if (focus == mWidget)
            mTransparent = true;
    }
    if (!mWidget->getAbsoluteCoord().inside(mousePos))
        mTransparent = false;

    if (mTransparent)
    {
        mWidget->setNeedMouseFocus(false); // Allow click-through
        setAlpha(std::max(0.13f, mWidget->getAlpha() - dt*5));
    }
    else
    {
        mWidget->setNeedMouseFocus(true);
        setAlpha(std::min(1.0f, mWidget->getAlpha() + dt*5));
    }
}

void NoDrop::setAlpha(float alpha)
{
    mWidget->setAlpha(alpha);
}
