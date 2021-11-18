#include "widget.hpp"

#include <SDL_events.h>

#include "text.hpp"
#include "textedit.hpp"
#include "window.hpp"

namespace LuaUi
{
    void WidgetExtension::triggerEvent(std::string_view name, const sol::object& argument = sol::nil) const
    {
        auto it = mCallbacks.find(name);
        if (it != mCallbacks.end())
            it->second(argument);
    }

    void WidgetExtension::create(lua_State* lua, MyGUI::Widget* self)
    {
        mLua = lua;
        mWidget = self;

        mWidget->eventChangeCoord += MyGUI::newDelegate(this, &WidgetExtension::updateChildrenCoord);

        initialize();
    }

    void WidgetExtension::initialize()
    {
        mAbsoluteCoord = MyGUI::IntCoord();
        mRelativeCoord = MyGUI::FloatCoord();
        mAnchor = MyGUI::FloatSize();
        mForcedCoord = MyGUI::IntCoord();

        // \todo might be more efficient to only register these if there are Lua callbacks
        mWidget->eventKeyButtonPressed += MyGUI::newDelegate(this, &WidgetExtension::keyPress);
        mWidget->eventKeyButtonReleased += MyGUI::newDelegate(this, &WidgetExtension::keyRelease);
        mWidget->eventMouseButtonClick += MyGUI::newDelegate(this, &WidgetExtension::mouseClick);
        mWidget->eventMouseButtonDoubleClick += MyGUI::newDelegate(this, &WidgetExtension::mouseDoubleClick);
        mWidget->eventMouseButtonPressed += MyGUI::newDelegate(this, &WidgetExtension::mousePress);
        mWidget->eventMouseButtonReleased += MyGUI::newDelegate(this, &WidgetExtension::mouseRelease);
        mWidget->eventMouseMove += MyGUI::newDelegate(this, &WidgetExtension::mouseMove);
        mWidget->eventMouseDrag += MyGUI::newDelegate(this, &WidgetExtension::mouseDrag);

        mWidget->eventMouseSetFocus += MyGUI::newDelegate(this, &WidgetExtension::focusGain);
        mWidget->eventMouseLostFocus += MyGUI::newDelegate(this, &WidgetExtension::focusLoss);
        mWidget->eventKeySetFocus += MyGUI::newDelegate(this, &WidgetExtension::focusGain);
        mWidget->eventKeyLostFocus += MyGUI::newDelegate(this, &WidgetExtension::focusLoss);
    }

    void WidgetExtension::destroy()
    {
        clearCallbacks();
        deinitialize();

        for (WidgetExtension* child : mContent)
            child->destroy();
    }

    void WidgetExtension::deinitialize()
    {
        mWidget->eventKeyButtonPressed.clear();
        mWidget->eventKeyButtonReleased.clear();
        mWidget->eventMouseButtonClick.clear();
        mWidget->eventMouseButtonDoubleClick.clear();
        mWidget->eventMouseButtonPressed.clear();
        mWidget->eventMouseButtonReleased.clear();
        mWidget->eventMouseMove.clear();
        mWidget->eventMouseDrag.m_event.clear();

        mWidget->eventMouseSetFocus.clear();
        mWidget->eventMouseLostFocus.clear();
        mWidget->eventKeySetFocus.clear();
        mWidget->eventKeyLostFocus.clear();
    }

    sol::table WidgetExtension::makeTable() const
    {
        return sol::table(mLua, sol::create);
    }

    sol::object WidgetExtension::keyEvent(MyGUI::KeyCode code) const
    {
        SDL_Keysym keySym;
        // MyGUI key codes are not one to one with SDL key codes
        // \todo refactor sdlmappings.cpp to map this back to SDL correctly
        keySym.sym = static_cast<SDL_Keycode>(code.getValue());
        keySym.scancode = SDL_GetScancodeFromKey(keySym.sym);
        keySym.mod = SDL_GetModState();
        return sol::make_object(mLua, keySym);
    }

    sol::object WidgetExtension::mouseEvent(int left, int top, MyGUI::MouseButton button = MyGUI::MouseButton::None) const
    {
        auto position = osg::Vec2f(left, top);
        auto absolutePosition = mWidget->getAbsolutePosition();
        auto offset = position - osg::Vec2f(absolutePosition.left, absolutePosition.top);
        sol::table table = makeTable();
        table["position"] = position;
        table["offset"] = offset;
        // \todo refactor sdlmappings.cpp to map this back to SDL properly
        table["button"] = button.getValue() + 1;
        return table;
    }

    void WidgetExtension::addChild(WidgetExtension* ext)
    {
        mContent.push_back(ext);
    }

    WidgetExtension* WidgetExtension::childAt(size_t index) const
    {
        return mContent.at(index);
    }

    void WidgetExtension::assignChild(size_t index, WidgetExtension* ext)
    {
        if (mContent.size() <= index)
            throw std::logic_error("Invalid widget child index");
        mContent[index] = ext;
    }

    WidgetExtension* WidgetExtension::eraseChild(size_t index)
    {
        if (mContent.size() <= index)
            throw std::logic_error("Invalid widget child index");
        auto it = mContent.begin() + index;
        WidgetExtension* ext = *it;
        mContent.erase(it);
        return ext;
    }

    void WidgetExtension::setCallback(const std::string& name, const LuaUtil::Callback& callback)
    {
        mCallbacks[name] = callback;
    }

    void WidgetExtension::clearCallbacks()
    {
        mCallbacks.clear();
    }

    void WidgetExtension::setProperty(std::string_view name, sol::object value)
    {
        if (!setPropertyRaw(name, value))
            Log(Debug::Error) << "Invalid value of property " << name
                              << ": " << LuaUtil::toString(value);
    }

    MyGUI::IntCoord WidgetExtension::forcedOffset()
    {
        return mForcedCoord;
    }

    void WidgetExtension::setForcedOffset(const MyGUI::IntCoord& offset)
    {
        mForcedCoord = offset;
    }

    void WidgetExtension::updateCoord()
    {
        mWidget->setCoord(calculateCoord());
    }

    bool WidgetExtension::setPropertyRaw(std::string_view name, sol::object value)
    {
        if (name == "position")
        {
            if (!value.is<osg::Vec2f>())
                return false;
            auto v = value.as<osg::Vec2f>();
            mAbsoluteCoord.left = v.x();
            mAbsoluteCoord.top = v.y();
        }
        else if (name == "size")
        {
            if (!value.is<osg::Vec2f>())
                return false;
            auto v = value.as<osg::Vec2f>();
            mAbsoluteCoord.width = v.x();
            mAbsoluteCoord.height = v.y();
        }
        else if (name == "relativePosition")
        {
            if (!value.is<osg::Vec2f>())
                return false;
            auto v = value.as<osg::Vec2f>();
            mRelativeCoord.left = v.x();
            mRelativeCoord.top = v.y();
        }
        else if (name == "relativeSize")
        {
            if (!value.is<osg::Vec2f>())
                return false;
            auto v = value.as<osg::Vec2f>();
            mRelativeCoord.width = v.x();
            mRelativeCoord.height = v.y();
        }
        else if (name == "anchor")
        {
            if (!value.is<osg::Vec2f>())
                return false;
            auto v = value.as<osg::Vec2f>();
            mAnchor.width = v.x();
            mAnchor.height = v.y();
        }
        else if (name == "visible")
        {
            if (!value.is<bool>())
                return false;
            mWidget->setVisible(value.as<bool>());
        }
        return true;
    }

    void WidgetExtension::updateChildrenCoord(MyGUI::Widget* _widget)
    {
        for (auto& child : mContent)
            child->updateCoord();
    }

    MyGUI::IntSize WidgetExtension::calculateSize()
    {
        const MyGUI::IntSize& parentSize = mWidget->getParentSize();
        MyGUI::IntSize newSize;
        newSize = mAbsoluteCoord.size() + mForcedCoord.size();
        newSize.width += mRelativeCoord.width * parentSize.width;
        newSize.height += mRelativeCoord.height * parentSize.height;
        return newSize;
    }

    MyGUI::IntPoint WidgetExtension::calculatePosition(const MyGUI::IntSize& size)
    {
        const MyGUI::IntSize& parentSize = mWidget->getParentSize();
        MyGUI::IntPoint newPosition;
        newPosition = mAbsoluteCoord.point() + mForcedCoord.point();
        newPosition.left += mRelativeCoord.left * parentSize.width - mAnchor.width * size.width;
        newPosition.top += mRelativeCoord.top * parentSize.height - mAnchor.height * size.height;
        return newPosition;
    }

    MyGUI::IntCoord WidgetExtension::calculateCoord()
    {
        MyGUI::IntCoord newCoord;
        newCoord = calculateSize();
        newCoord = calculatePosition(newCoord.size());
        return newCoord;
    }

    void WidgetExtension::keyPress(MyGUI::Widget*, MyGUI::KeyCode code, MyGUI::Char ch)
    {
        if (code == MyGUI::KeyCode::None)
        {
            // \todo decide how to handle unicode strings in Lua
            MyGUI::UString uString;
            uString.push_back(static_cast<MyGUI::UString::unicode_char>(ch));
            triggerEvent("textInput", sol::make_object(mLua, uString.asUTF8()));
        }
        else
            triggerEvent("keyPress", keyEvent(code));
    }

    void WidgetExtension::keyRelease(MyGUI::Widget*, MyGUI::KeyCode code)
    {
        triggerEvent("keyRelease", keyEvent(code));
    }

    void WidgetExtension::mouseMove(MyGUI::Widget*, int left, int top)
    {
        triggerEvent("mouseMove", mouseEvent(left, top));
    }

    void WidgetExtension::mouseDrag(MyGUI::Widget*, int left, int top, MyGUI::MouseButton button)
    {
        triggerEvent("mouseMove", mouseEvent(left, top, button));
    }

    void WidgetExtension::mouseClick(MyGUI::Widget* _widget)
    {
        triggerEvent("mouseClick");
    }

    void WidgetExtension::mouseDoubleClick(MyGUI::Widget* _widget)
    {
        triggerEvent("mouseDoubleClick");
    }

    void WidgetExtension::mousePress(MyGUI::Widget*, int left, int top, MyGUI::MouseButton button)
    {
        triggerEvent("mousePress", mouseEvent(left, top, button));
    }

    void WidgetExtension::mouseRelease(MyGUI::Widget*, int left, int top, MyGUI::MouseButton button)
    {
        triggerEvent("mouseRelease", mouseEvent(left, top, button));
    }

    void WidgetExtension::focusGain(MyGUI::Widget*, MyGUI::Widget*)
    {
        triggerEvent("focusGain");
    }

    void WidgetExtension::focusLoss(MyGUI::Widget*, MyGUI::Widget*)
    {
        triggerEvent("focusLoss");
    }
}
