#include "keyboardnavigation.hpp"

#include <MyGUI_InputManager.h>
#include <MyGUI_WidgetManager.h>
#include <MyGUI_Button.h>

#include "../mwbase/windowmanager.hpp"
#include "../mwbase/environment.hpp"

namespace MWGui
{

/// Recursively get all child widgets that accept keyboard input
void getKeyFocusWidgets(MyGUI::Widget* parent, std::vector<MyGUI::Widget*>& results)
{
    MyGUI::EnumeratorWidgetPtr enumerator = parent->getEnumerator();
    while (enumerator.next())
    {
        MyGUI::Widget* w = enumerator.current();
        if (!w->getVisible() || !w->getEnabled())
            continue;
        if (w->getNeedKeyFocus())
            results.push_back(w);
        else
            getKeyFocusWidgets(w, results);
    }
}

KeyboardNavigation::KeyboardNavigation()
{
}

KeyboardNavigation::~KeyboardNavigation()
{
}

bool isButtonFocus()
{
    MyGUI::Widget* focus = MyGUI::InputManager::getInstance().getKeyFocusWidget();
    return focus->getTypeName().find("Button") != std::string::npos;
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

bool KeyboardNavigation::injectKeyPress(MyGUI::KeyCode key, unsigned int text)
{
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
        return accept();
    default:
        return false;
    }
}

bool KeyboardNavigation::switchFocus(int direction, bool wrap)
{
    MyGUI::Widget* focus = MyGUI::InputManager::getInstance().getKeyFocusWidget();
    if (!focus)
        return false;

    if (!isButtonFocus() && direction != D_Prev && direction != D_Next)
        return false;

    MyGUI::Widget* window = MyGUI::InputManager::getInstance().getKeyFocusWidget();
    while (window->getParent())
        window = window->getParent();

    MyGUI::VectorWidgetPtr keyFocusList;
    getKeyFocusWidgets(window, keyFocusList);

    if (keyFocusList.empty())
        return false;

    MyGUI::VectorWidgetPtr::iterator found = std::find(keyFocusList.begin(), keyFocusList.end(), focus);
    if (found == keyFocusList.end())
        return false;

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
    if (direction == D_Right && horizdiff <= 0)
        return false;
    else if (direction == D_Left && horizdiff >= 0)
        return false;
    else if (direction == D_Down && vertdiff <= 0)
        return false;
    else if (direction == D_Up && vertdiff >= 0)
        return false;

    MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(keyFocusList[index]);
    return true;
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
