#include "windowpinnablebase.hpp"

#include <MyGUI_Button.h>

#include "exposedwindow.hpp"

namespace MWGui
{
    WindowPinnableBase::WindowPinnableBase(const std::string& parLayout)
      : WindowBase(parLayout), mPinned(false)
    {
        ExposedWindow* window = mMainWidget->castType<ExposedWindow>();
        mPinButton = window->getSkinWidget ("Button");

        mPinButton->eventMouseButtonPressed += MyGUI::newDelegate(this, &WindowPinnableBase::onPinButtonPressed);

        MyGUI::Button* button = nullptr;
        MyGUI::VectorWidgetPtr widgets = window->getSkinWidgetsByName("Action");
        for (MyGUI::Widget* widget : widgets)
        {
            if (widget->isUserString("HideWindowOnDoubleClick"))
                button = widget->castType<MyGUI::Button>();
        }

        if (button)
            button->eventMouseButtonDoubleClick += MyGUI::newDelegate(this, &WindowPinnableBase::onDoubleClick);
    }

    void WindowPinnableBase::onPinButtonPressed(MyGUI::Widget* _sender, int left, int top, MyGUI::MouseButton id)
    {
        if (id != MyGUI::MouseButton::Left)
            return;

        mPinned = !mPinned;

        if (mPinned)
            mPinButton->changeWidgetSkin ("PinDown");
        else
            mPinButton->changeWidgetSkin ("PinUp");

        onPinToggled();
    }

    void WindowPinnableBase::onDoubleClick(MyGUI::Widget *_sender)
    {
        onTitleDoubleClicked();
    }

    void WindowPinnableBase::setPinned(bool pinned)
    {
        if (pinned != mPinned)
            onPinButtonPressed(mPinButton, 0, 0, MyGUI::MouseButton::Left);
    }

    void WindowPinnableBase::setPinButtonVisible(bool visible)
    {
        mPinButton->setVisible(visible);
    }
}
