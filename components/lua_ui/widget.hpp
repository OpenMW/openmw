#ifndef OPENMW_LUAUI_WIDGET
#define OPENMW_LUAUI_WIDGET

#include <map>

#include <MyGUI_Widget.h>
#include <sol/sol.hpp>
#include <osg/Vec2>

#include <components/lua/scriptscontainer.hpp>

namespace LuaUi
{
    /*
    * extends MyGUI::Widget and its child classes
    * memory ownership is controlled by MyGUI
    * it is important not to call any WidgetExtension methods after destroying the MyGUI::Widget
    */
    class WidgetExtension
    {
    public:
        WidgetExtension();
        // must be called after creating the underlying MyGUI::Widget
        void create(lua_State* lua, MyGUI::Widget* self);
        // must be called after before destroying the underlying MyGUI::Widget
        void destroy();

        void addChild(WidgetExtension* ext);
        WidgetExtension* childAt(size_t index) const;
        void assignChild(size_t index, WidgetExtension* ext);
        WidgetExtension* eraseChild(size_t index);
        size_t childCount() const { return mContent.size(); }

        MyGUI::Widget* widget() const { return mWidget; }

        void setCallback(const std::string&, const LuaUtil::Callback&);
        void clearCallbacks();

        void setProperty(std::string_view, sol::object value);

        MyGUI::IntCoord forcedOffset();
        void setForcedOffset(const MyGUI::IntCoord& offset);
        void updateCoord();

    protected:
        sol::table makeTable() const;
        sol::object keyEvent(MyGUI::KeyCode) const;
        sol::object mouseEvent(int left, int top, MyGUI::MouseButton button) const;
        virtual bool setPropertyRaw(std::string_view name, sol::object value);
        virtual void initialize();
        virtual void deinitialize();
        virtual MyGUI::IntSize calculateSize();
        virtual MyGUI::IntPoint calculatePosition(const MyGUI::IntSize& size);
        MyGUI::IntCoord calculateCoord();

        void triggerEvent(std::string_view name, const sol::object& argument) const;

        MyGUI::IntCoord mForcedCoord;
        MyGUI::IntCoord mAbsoluteCoord;
        MyGUI::FloatCoord mRelativeCoord;
        MyGUI::FloatSize mAnchor;

    private:
        // use lua_State* instead of sol::state_view because MyGUI requires a default constructor
        lua_State* mLua; 
        MyGUI::Widget* mWidget;

        std::vector<WidgetExtension*> mContent;
        std::map<std::string, LuaUtil::Callback, std::less<>> mCallbacks;

        void updateChildrenCoord(MyGUI::Widget*);

        void keyPress(MyGUI::Widget*, MyGUI::KeyCode, MyGUI::Char);
        void keyRelease(MyGUI::Widget*, MyGUI::KeyCode);
        void mouseMove(MyGUI::Widget*, int, int);
        void mouseDrag(MyGUI::Widget*, int, int, MyGUI::MouseButton);
        void mouseClick(MyGUI::Widget*);
        void mouseDoubleClick(MyGUI::Widget*);
        void mousePress(MyGUI::Widget*, int, int, MyGUI::MouseButton);
        void mouseRelease(MyGUI::Widget*, int, int, MyGUI::MouseButton);
        void focusGain(MyGUI::Widget*, MyGUI::Widget*);
        void focusLoss(MyGUI::Widget*, MyGUI::Widget*);
    };

    class LuaWidget : public MyGUI::Widget, public WidgetExtension
    {
        MYGUI_RTTI_DERIVED(LuaWidget)
    };
}

#endif // !OPENMW_LUAUI_WIDGET
