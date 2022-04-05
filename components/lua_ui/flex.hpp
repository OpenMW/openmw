#ifndef OPENMW_LUAUI_FLEX
#define OPENMW_LUAUI_FLEX

#include "widget.hpp"
#include "alignment.hpp"

namespace LuaUi
{
    class LuaFlex : public MyGUI::Widget, public WidgetExtension
    {
        MYGUI_RTTI_DERIVED(LuaFlex)

        protected:
            MyGUI::IntSize calculateSize() override;
            void updateProperties() override;
            void updateChildren() override;
            MyGUI::IntSize childScalingSize() override
            {
                return MyGUI::IntSize();
            }

        private:
            bool mHorizontal;
            bool mAutoSized;
            MyGUI::IntSize mChildrenSize;
            Alignment mAlign;
            Alignment mArrange;

            template<typename T>
            inline T getPrimary(const MyGUI::types::TPoint<T>& point)
            {
                return mHorizontal ? point.left : point.top;
            }

            template<typename T>
            inline T getSecondary(const MyGUI::types::TPoint<T>& point)
            {
                return mHorizontal ? point.top : point.left;
            }

            template<typename T>
            inline void setPrimary(MyGUI::types::TPoint<T>& point, T value)
            {
                if (mHorizontal)
                    point.left = value;
                else
                    point.top = value;
            }

            template<typename T>
            inline void setSecondary(MyGUI::types::TPoint<T>& point, T value)
            {
                if (mHorizontal)
                    point.top = value;
                else
                    point.left = value;
            }

            template<typename T>
            inline T getPrimary(const MyGUI::types::TSize<T>& point)
            {
                return mHorizontal ? point.width : point.height;
            }

            template<typename T>
            inline T getSecondary(const MyGUI::types::TSize<T>& point)
            {
                return mHorizontal ? point.height : point.width;
            }

            template<typename T>
            inline void setPrimary(MyGUI::types::TSize<T>& point, T value)
            {
                if (mHorizontal)
                    point.width = value;
                else
                    point.height = value;
            }

            template<typename T>
            inline void setSecondary(MyGUI::types::TSize<T>& point, T value)
            {
                if (mHorizontal)
                    point.height = value;
                else
                    point.width = value;
            }
    };
}

#endif // OPENMW_LUAUI_FLEX
