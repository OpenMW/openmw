#ifndef OPENMW_LUAUI_TEXTEDIT
#define OPENMW_LUAUI_TEXTEDIT

#include <string>

#include <MyGUI_RTTI.h>
#include <MyGUI_Types.h>
#include <MyGUI_Widget.h>

#include "widget.hpp"

namespace MyGUI
{
    class EditBox;
}

namespace LuaUi
{
    class LuaTextEdit : public MyGUI::Widget, public WidgetExtension
    {
        MYGUI_RTTI_DERIVED(LuaTextEdit)

    protected:
        void initialize() override;
        void deinitialize() override;
        void updateProperties() override;
        void updateCoord() override;
        void updateChildren() override;
        MyGUI::IntSize calculateSize() override;

    private:
        void textChange(MyGUI::EditBox*);

        MyGUI::EditBox* mEditBox = nullptr;
        bool mMultiline{ false };
        bool mAutoSize{ false };
    };
}

#endif // OPENMW_LUAUI_TEXTEDIT
