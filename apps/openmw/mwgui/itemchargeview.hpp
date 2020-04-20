#ifndef MWGUI_ITEMCHARGEVIEW_H
#define MWGUI_ITEMCHARGEVIEW_H

#include <vector>
#include <memory>

#include <MyGUI_Widget.h>

#include "../mwworld/ptr.hpp"

#include "widgets.hpp"

namespace MyGUI
{
    class TextBox;
    class ScrollView;
}

namespace MWGui
{
    class ItemModel;
    class ItemWidget;

    class ItemChargeView final : public MyGUI::Widget
    {
        MYGUI_RTTI_DERIVED(ItemChargeView)
        public:
            enum DisplayMode
            {
                DisplayMode_Health,
                DisplayMode_EnchantmentCharge
            };

            ItemChargeView();

            /// Register needed components with MyGUI's factory manager
            static void registerComponents();

            void initialiseOverride() final;

            /// Takes ownership of \a model
            void setModel(ItemModel* model);

            void setDisplayMode(DisplayMode type);

            void update();
            void layoutWidgets();
            void resetScrollbars();

            void setSize(const MyGUI::IntSize& value) final;
            void setCoord(const MyGUI::IntCoord& value) final;

            MyGUI::delegates::CMultiDelegate2<MyGUI::Widget*, const MWWorld::Ptr&> eventItemClicked;

        private:
            struct Line
            {
                MWWorld::Ptr mItemPtr;
                MyGUI::TextBox* mText;
                ItemWidget* mIcon;
                Widgets::MWDynamicStatPtr mCharge;
            };

            void updateLine(const Line& line);

            void onIconClicked(MyGUI::Widget* sender);
            void onMouseWheelMoved(MyGUI::Widget* sender, int rel);

            typedef std::vector<Line> Lines;
            Lines mLines;

            std::unique_ptr<ItemModel> mModel;
            MyGUI::ScrollView* mScrollView;
            DisplayMode mDisplayMode;
    };
}

#endif
