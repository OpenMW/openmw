#include "keyboardnavigation.hpp"

#include <MyGUI_InputManager.h>
#include <MyGUI_WidgetManager.h>
#include <MyGUI_Button.h>
#include <MyGUI_Gui.h>
#include <MyGUI_Window.h>

#include <components/debug/debuglog.hpp>

#include "../mwbase/windowmanager.hpp"
#include "../mwbase/environment.hpp"

namespace MWGui
{

bool shouldAcceptKeyFocus(MyGUI::Widget* w)
{
    return w && !w->castType<MyGUI::Window>(false) && w->getInheritedEnabled() && w->getInheritedVisible() && w->getVisible() && w->getEnabled();
}

/// Recursively get all child widgets that accept keyboard input
void getKeyFocusWidgets(MyGUI::Widget* parent, std::vector<MyGUI::Widget*>& results)
{
    assert(parent != nullptr);

    if (!parent->getVisible() || !parent->getEnabled())
        return;

    MyGUI::EnumeratorWidgetPtr enumerator = parent->getEnumerator();
    while (enumerator.next())
    {
        MyGUI::Widget* w = enumerator.current();
        if (!w->getVisible() || !w->getEnabled())
            continue;
        if (w->getNeedKeyFocus() && shouldAcceptKeyFocus(w))
            results.push_back(w);
        else
            getKeyFocusWidgets(w, results);
    }
}

KeyboardNavigation::KeyboardNavigation()
    : mCurrentFocus(nullptr)
    , mModalWindow(nullptr)
    , mEnabled(true)
{
    MyGUI::WidgetManager::getInstance().registerUnlinker(this);
}

KeyboardNavigation::~KeyboardNavigation()
{
    try
    {
        MyGUI::WidgetManager::getInstance().unregisterUnlinker(this);
    }
    catch(const MyGUI::Exception& e)
    {
        Log(Debug::Error) << "Error in the destructor: " << e.what();
    }
}

void KeyboardNavigation::saveFocus(int mode)
{
    MyGUI::Widget* focus = MyGUI::InputManager::getInstance().getKeyFocusWidget();
    if (shouldAcceptKeyFocus(focus))
    {
        mKeyFocus[mode] = focus;
    }
    else
    {
        mKeyFocus[mode] = mCurrentFocus;
    }
}

void KeyboardNavigation::restoreFocus(int mode)
{
    std::map<int, MyGUI::Widget*>::const_iterator found = mKeyFocus.find(mode);
    if (found != mKeyFocus.end())
    {
        MyGUI::Widget* w = found->second;
        if (w && w->getVisible() && w->getEnabled())
            MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(found->second);
    }
}

void KeyboardNavigation::_unlinkWidget(MyGUI::Widget *widget)
{
    for (std::pair<const int, MyGUI::Widget*>& w : mKeyFocus)
        if (w.second == widget)
            w.second = nullptr;
    if (widget == mCurrentFocus)
        mCurrentFocus = nullptr;
}

void styleFocusedButton(MyGUI::Widget* w)
{
    if (w)
    {
        if (MyGUI::Button* b = w->castType<MyGUI::Button>(false))
        {
            b->_setWidgetState("highlighted");
        }
    }
}

bool isRootParent(MyGUI::Widget* widget, MyGUI::Widget* root)
{
    while (widget && widget->getParent())
        widget = widget->getParent();
    return widget == root;
}

void KeyboardNavigation::onFrame()
{
    if (!mEnabled)
        return;

    if (!MWBase::Environment::get().getWindowManager()->isGuiMode())
    {
        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(nullptr);
        return;
    }

    MyGUI::Widget* focus = MyGUI::InputManager::getInstance().getKeyFocusWidget();

    if (focus == mCurrentFocus)
    {
        styleFocusedButton(mCurrentFocus);
        return;
    }

    // workaround incorrect key focus resets (fix in MyGUI TBD)
    if (!shouldAcceptKeyFocus(focus) && shouldAcceptKeyFocus(mCurrentFocus) && (!mModalWindow || isRootParent(mCurrentFocus, mModalWindow)))
    {
        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mCurrentFocus);
        focus = mCurrentFocus;
    }

    // style highlighted button (won't be needed for MyGUI 3.2.3)
    if (focus != mCurrentFocus)
    {
        if (mCurrentFocus)
        {
            if (MyGUI::Button* b = mCurrentFocus->castType<MyGUI::Button>(false))
                b->_setWidgetState("normal");
        }

        mCurrentFocus = focus;
    }

    styleFocusedButton(mCurrentFocus);
}

void KeyboardNavigation::setDefaultFocus(MyGUI::Widget *window, MyGUI::Widget *defaultFocus)
{
    MyGUI::Widget* focus = MyGUI::InputManager::getInstance().getKeyFocusWidget();
    if (!focus || !shouldAcceptKeyFocus(focus))
    {
        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(defaultFocus);
    }
    else
    {
        if (!isRootParent(focus, window))
            MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(defaultFocus);
    }
}

void KeyboardNavigation::setModalWindow(MyGUI::Widget *window)
{
    mModalWindow = window;
}

void KeyboardNavigation::setEnabled(bool enabled)
{
    mEnabled = enabled;
}

enum Direction
{
    D_Left,
    D_Up,
    D_Right,
    D_Down,
    D_Next,
    D_Prev
};

bool KeyboardNavigation::injectKeyPress(MyGUI::KeyCode key, unsigned int text, bool repeat)
{
    if (!mEnabled)
        return false;

    switch (key.getValue())
    {
    case MyGUI::KeyCode::ArrowLeft:
        return switchFocus(D_Left, false);
    case MyGUI::KeyCode::ArrowRight:
        return switchFocus(D_Right, false);
    case MyGUI::KeyCode::ArrowUp:
        return switchFocus(D_Up, false);
    case MyGUI::KeyCode::ArrowDown:
        return switchFocus(D_Down, false);
    case MyGUI::KeyCode::Tab:
        return switchFocus(MyGUI::InputManager::getInstance().isShiftPressed() ? D_Prev : D_Next, true);
    case MyGUI::KeyCode::Return:
    case MyGUI::KeyCode::NumpadEnter:
    case MyGUI::KeyCode::Space:
    {
        // We should disable repeating for activation keys
        MyGUI::InputManager::getInstance().injectKeyRelease(MyGUI::KeyCode::None);
        if (repeat)
            return true;

        return accept();
    }
    default:
        return false;
    }
}

bool KeyboardNavigation::switchFocus(int direction, bool wrap)
{
    if (!MWBase::Environment::get().getWindowManager()->isGuiMode())
        return false;

    MyGUI::Widget* focus = MyGUI::InputManager::getInstance().getKeyFocusWidget();

    bool isCycle = (direction == D_Prev || direction == D_Next);

    if ((focus && focus->getTypeName().find("Button") == std::string::npos) && !isCycle)
        return false;

    if (focus && isCycle && focus->getUserString("AcceptTab") == "true")
        return false;

    if ((!focus || !focus->getNeedKeyFocus()) && isCycle)
    {
        // if nothing is selected, select the first widget
        return selectFirstWidget();
    }
    if (!focus)
        return false;

    MyGUI::Widget* window = focus;
    while (window && window->getParent())
        window = window->getParent();
    MyGUI::VectorWidgetPtr keyFocusList;
    getKeyFocusWidgets(window, keyFocusList);

    if (keyFocusList.empty())
        return false;

    MyGUI::VectorWidgetPtr::iterator found = std::find(keyFocusList.begin(), keyFocusList.end(), focus);
    if (found == keyFocusList.end())
    {
        if (isCycle)
            return selectFirstWidget();
        else
            return false;
    }

    bool forward = (direction == D_Next || direction == D_Right || direction == D_Down);

    int index = found - keyFocusList.begin();
    index = forward ? (index+1) : (index-1);
    if (wrap)
        index = (index + keyFocusList.size())%keyFocusList.size();
    else
        index = std::min(std::max(0, index), static_cast<int>(keyFocusList.size())-1);

    MyGUI::Widget* next = keyFocusList[index];
    int vertdiff = next->getTop() - focus->getTop();
    int horizdiff = next->getLeft() - focus->getLeft();
    bool isVertical = std::abs(vertdiff) > std::abs(horizdiff);
    if (direction == D_Right && (horizdiff <= 0 || isVertical))
        return false;
    else if (direction == D_Left && (horizdiff >= 0 || isVertical))
        return false;
    else if (direction == D_Down && (vertdiff <= 0 || !isVertical))
        return false;
    else if (direction == D_Up && (vertdiff >= 0 || !isVertical))
        return false;

    MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(keyFocusList[index]);
    return true;
}

bool KeyboardNavigation::selectFirstWidget()
{
    MyGUI::VectorWidgetPtr keyFocusList;
    MyGUI::EnumeratorWidgetPtr enumerator = MyGUI::Gui::getInstance().getEnumerator();
    if (mModalWindow)
        enumerator = mModalWindow->getEnumerator();
    while (enumerator.next())
        getKeyFocusWidgets(enumerator.current(), keyFocusList);

    if (!keyFocusList.empty())
    {
        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(keyFocusList[0]);
        return true;
    }
    return false;
}

bool KeyboardNavigation::accept()
{
    MyGUI::Widget* focus = MyGUI::InputManager::getInstance().getKeyFocusWidget();
    if (!focus)
        return false;
    //MyGUI::Button* button = focus->castType<MyGUI::Button>(false);
    //if (button && button->getEnabled())
    if (focus->getTypeName().find("Button") != std::string::npos && focus->getEnabled())
    {
        focus->eventMouseButtonClick(focus);
        return true;
    }
    return false;
}


}
