#ifndef OPENMW_LUAUI_FLEX
#define OPENMW_LUAUI_FLEX

#include "alignment.hpp"
#include "widget.hpp"

namespace LuaUi
{
    class LuaFlex : public MyGUI::Widget, public WidgetExtension
    {
        MYGUI_RTTI_DERIVED(LuaFlex)

    protected:
        MyGUI::IntSize calculateSize() const override;
        void updateProperties() override;
        void updateChildren() override;
        MyGUI::IntSize childScalingSize() const override;

        void updateCoord() override;

    private:
        bool mHorizontal;
        bool mAutoSized;
        MyGUI::IntSize mChildrenSize;
        Alignment mAlign;
        Alignment mArrange;

        template <typename T>
        T& primary(MyGUI::types::TPoint<T>& point) const
        {
            return mHorizontal ? point.left : point.top;
        }

        template <typename T>
        T& secondary(MyGUI::types::TPoint<T>& point) const
        {
            return mHorizontal ? point.top : point.left;
        }

        template <typename T>
        T& primary(MyGUI::types::TSize<T>& size) const
        {
            return mHorizontal ? size.width : size.height;
        }

        template <typename T>
        T& secondary(MyGUI::types::TSize<T>& size) const
        {
            return mHorizontal ? size.height : size.width;
        }

        template <typename T>
        T primary(const MyGUI::types::TSize<T>& size) const
        {
            return mHorizontal ? size.width : size.height;
        }

        template <typename T>
        T secondary(const MyGUI::types::TSize<T>& size) const
        {
            return mHorizontal ? size.height : size.width;
        }
    };
}

#endif // OPENMW_LUAUI_FLEX
