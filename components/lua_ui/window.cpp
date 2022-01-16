#include "window.hpp"

#include <MyGUI_InputManager.h>

namespace LuaUi
{
    LuaWindow::LuaWindow()
        : mCaption()
        , mPreviousMouse()
        , mChangeScale()
        , mMoveResize()
    {}

    void LuaWindow::initialize()
    {
        WidgetExtension::initialize();

        assignWidget(mCaption, "Caption");
        if (mCaption)
        {
            mCaption->eventMouseButtonPressed += MyGUI::newDelegate(this, &LuaWindow::notifyMousePress);
            mCaption->eventMouseDrag += MyGUI::newDelegate(this, &LuaWindow::notifyMouseDrag);
        }
        for (auto w : getSkinWidgetsByName("Action"))
        {
            w->eventMouseButtonPressed += MyGUI::newDelegate(this, &LuaWindow::notifyMousePress);
            w->eventMouseDrag += MyGUI::newDelegate(this, &LuaWindow::notifyMouseDrag);
        }
    }

    void LuaWindow::deinitialize()
    {
        WidgetExtension::deinitialize();

        if (mCaption)
        {
            mCaption->eventMouseButtonPressed.clear();
            mCaption->eventMouseDrag.m_event.clear();
        }
        for (auto w : getSkinWidgetsByName("Action"))
        {
            w->eventMouseButtonPressed.clear();
            w->eventMouseDrag.m_event.clear();
        }
    }

    void LuaWindow::setProperties(sol::object props)
    {
        if (mCaption)
            mCaption->setCaption(parseProperty(props, "caption", std::string()));
        mMoveResize = MyGUI::IntCoord();
        setForcedCoord(mMoveResize);
        WidgetExtension::setProperties(props);
    }

    void LuaWindow::notifyMousePress(MyGUI::Widget* sender, int left, int top, MyGUI::MouseButton id)
    {
        if (id != MyGUI::MouseButton::Left)
            return;

        mPreviousMouse.left = left;
        mPreviousMouse.top = top;

        if (sender->isUserString("Scale"))
            mChangeScale = MyGUI::IntCoord::parse(sender->getUserString("Scale"));
        else
            mChangeScale = MyGUI::IntCoord(1, 1, 0, 0);
    }

    void LuaWindow::notifyMouseDrag(MyGUI::Widget* sender, int left, int top, MyGUI::MouseButton id)
    {
        if (id != MyGUI::MouseButton::Left)
            return;

        MyGUI::IntCoord change = mChangeScale;
        change.left *= (left - mPreviousMouse.left);
        change.top *= (top - mPreviousMouse.top);
        change.width *= (left - mPreviousMouse.left);
        change.height *= (top - mPreviousMouse.top);

        mMoveResize = mMoveResize + change.size();
        setForcedCoord(mMoveResize);
        // position can change based on size changes
        mMoveResize = mMoveResize + change.point() + getPosition() - calculateCoord().point();
        setForcedCoord(mMoveResize);
        updateCoord();

        mPreviousMouse.left = left;
        mPreviousMouse.top = top;

        sol::table table = makeTable();
        table["position"] = osg::Vec2f(mCoord.left, mCoord.top);
        table["size"] = osg::Vec2f(mCoord.width, mCoord.height);
        triggerEvent("windowDrag", table);
    }
}
