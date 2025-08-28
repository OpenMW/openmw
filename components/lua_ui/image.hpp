#ifndef OPENMW_LUAUI_IMAGE
#define OPENMW_LUAUI_IMAGE

#include <MyGUI_ImageBox.h>
#include <MyGUI_TileRect.h>

#include "widget.hpp"

namespace LuaUi
{
    class LuaTileRect : public MyGUI::TileRect
    {
        MYGUI_RTTI_DERIVED(LuaTileRect)

    public:
        void _setAlign(const MyGUI::IntSize& oldSize) override;

        void updateSize(MyGUI::IntSize tileSize) { mSetTileSize = tileSize; }

    protected:
        MyGUI::IntSize mSetTileSize;
    };

    class LuaImage : public MyGUI::ImageBox, public WidgetExtension
    {
        MYGUI_RTTI_DERIVED(LuaImage)

    protected:
        void initialize() override;
        void updateProperties() override;
        LuaTileRect* mTileRect;
    };
}

#endif // OPENMW_LUAUI_IMAGE
