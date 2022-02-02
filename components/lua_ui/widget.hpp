#ifndef OPENMW_LUAUI_WIDGET
#define OPENMW_LUAUI_WIDGET

#include <map>
#include <functional>

#include <MyGUI_Widget.h>
#include <sol/sol.hpp>

#include <components/lua/scriptscontainer.hpp>

#include "properties.hpp"

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
        void initialize(lua_State* lua, MyGUI::Widget* self);
        // must be called after before destroying the underlying MyGUI::Widget
        virtual void deinitialize();

        MyGUI::Widget* widget() const { return mWidget; }

        const std::vector<WidgetExtension*>& children() { return mChildren; }
        void setChildren(const std::vector<WidgetExtension*>&);

        const std::vector<WidgetExtension*>& templateChildren() { return mTemplateChildren; }
        void setTemplateChildren(const std::vector<WidgetExtension*>&);

        void setCallback(const std::string&, const LuaUtil::Callback&);
        void clearCallbacks();

        void setProperties(sol::object);
        void setTemplateProperties(sol::object props) { mTemplateProperties = props; }

        void setExternal(sol::object external) { mExternal = external; }

        MyGUI::IntCoord forcedCoord();
        void setForcedCoord(const MyGUI::IntCoord& offset);
        void setForcedSize(const MyGUI::IntSize& size);
        void updateCoord();

        const sol::table& getLayout() { return mLayout; }
        void setLayout(const sol::table& layout) { mLayout = layout; }

        template <typename T>
        T externalValue(std::string_view name, const T& defaultValue)
        {
            return parseExternal(mExternal, name, defaultValue);
        }

        void onCoordChange(const std::optional<std::function<void(WidgetExtension*, MyGUI::IntCoord)>>& callback)
        {
            mOnCoordChange = callback;
        }

    protected:
        virtual void initialize();
        sol::table makeTable() const;
        sol::object keyEvent(MyGUI::KeyCode) const;
        sol::object mouseEvent(int left, int top, MyGUI::MouseButton button) const;

        MyGUI::IntSize parentSize();
        virtual MyGUI::IntSize calculateSize();
        virtual MyGUI::IntPoint calculatePosition(const MyGUI::IntSize& size);
        MyGUI::IntCoord calculateCoord();
        virtual MyGUI::IntSize childScalingSize();

        template<typename T>
        T propertyValue(std::string_view name, const T& defaultValue)
        {
            return parseProperty(mProperties, mTemplateProperties, name, defaultValue);
        }

        WidgetExtension* findFirstInTemplates(std::string_view flagName);
        std::vector<WidgetExtension*> findAllInTemplates(std::string_view flagName);

        virtual void updateTemplate();
        virtual void updateProperties();
        virtual void updateChildren() {};

        void triggerEvent(std::string_view name, const sol::object& argument) const;

        // offsets the position and size, used only in C++ widget code
        MyGUI::IntCoord mForcedCoord;
        // position and size in pixels
        MyGUI::IntCoord mAbsoluteCoord;
        // position and size as a ratio of parent size
        MyGUI::FloatCoord mRelativeCoord;
        // negative position offset as a ratio of this widget's size
        // used in combination with relative coord to align the widget, e. g. center it
        MyGUI::FloatSize mAnchor;

    private:
        // use lua_State* instead of sol::state_view because MyGUI requires a default constructor
        lua_State* mLua;
        MyGUI::Widget* mWidget;
        std::vector<WidgetExtension*> mChildren;
        std::vector<WidgetExtension*> mTemplateChildren;
        WidgetExtension* mSlot;
        std::map<std::string, LuaUtil::Callback, std::less<>> mCallbacks;
        sol::table mLayout;
        sol::object mProperties;
        sol::object mTemplateProperties;
        sol::object mExternal;
        WidgetExtension* mParent;

        void attach(WidgetExtension* ext);

        WidgetExtension* findFirst(std::string_view name);
        void findAll(std::string_view flagName, std::vector<WidgetExtension*>& result);

        void updateChildrenCoord();

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

        std::optional<std::function<void(WidgetExtension*, MyGUI::IntCoord)>> mOnCoordChange;
    };

    class LuaWidget : public MyGUI::Widget, public WidgetExtension
    {
        MYGUI_RTTI_DERIVED(LuaWidget)
    };
}

#endif // !OPENMW_LUAUI_WIDGET
