#ifndef OPENMW_LUAUI_TEXT
#define OPENMW_LUAUI_TEXT

#include <vector>

#include <MyGUI_EditBox.h>

#include "widget.hpp"

namespace LuaUi
{
    class LuaText : public MyGUI::EditBox, public WidgetExtension
    {
        MYGUI_RTTI_DERIVED(LuaText)

    public:
        LuaText();
        void initialize() override;
        void updateProperties() override;
        void setCaption(const MyGUI::UString& caption) override;

        const std::vector<std::string_view>& allUsedProperties() const override;

    private:
        bool mAutoSized;

    protected:
        MyGUI::IntSize calculateSize() const override;
    };
}

#endif // OPENMW_LUAUI_TEXT
