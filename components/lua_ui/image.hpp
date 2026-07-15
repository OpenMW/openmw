#ifndef OPENMW_LUAUI_IMAGE
#define OPENMW_LUAUI_IMAGE

#include <vector>

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
        void updatePadding(const osg::Vec4i& padding) { mPadding = padding; }

    protected:
        MyGUI::IntSize mSetTileSize;
        // top, right, bottom, left
        osg::Vec4i mPadding{ 0, 0, 0, 0 };
    };

    class LuaImage : public MyGUI::ImageBox, public WidgetExtension
    {
        MYGUI_RTTI_DERIVED(LuaImage)

    protected:
        void initialize() override;
        void updateProperties() override;
        const std::vector<std::string_view>& allUsedProperties() const override;
        LuaTileRect* mTileRect;
    };
}

#endif // OPENMW_LUAUI_IMAGE
