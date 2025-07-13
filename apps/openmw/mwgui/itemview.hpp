#ifndef MWGUI_ITEMVIEW_H
#define MWGUI_ITEMVIEW_H

#include <MyGUI_Widget.h>
#include <SDL.h>

#include "itemmodel.hpp"

namespace MWGui
{

    class ItemView final : public MyGUI::Widget
    {
        MYGUI_RTTI_DERIVED(ItemView)
    public:
        ItemView();

        /// Register needed components with MyGUI's factory manager
        static void registerComponents();

        ItemModel* getModel() { return mModel.get(); }

        /// Takes ownership of \a model
        void setModel(std::unique_ptr<ItemModel> model);

        typedef MyGUI::delegates::MultiDelegate<ItemModel::ModelIndex> EventHandle_ModelIndex;
        typedef MyGUI::delegates::MultiDelegate<> EventHandle_Void;
        /// Fired when an item was clicked
        EventHandle_ModelIndex eventItemClicked;
        /// Fired when the background was clicked (useful for drag and drop)
        EventHandle_Void eventBackgroundClicked;

        void update();

        void resetScrollBars();

        void setActiveControllerWindow(bool active);
        int getControllerFocus() { return mControllerFocus; }
        int getItemCount() { return mItemCount; }
        void onControllerButton(const unsigned char button);

    private:
        void initialiseOverride() override;

        void layoutWidgets();

        void setSize(const MyGUI::IntSize& _value) override;
        void setCoord(const MyGUI::IntCoord& _value) override;

        void onSelectedItem(MyGUI::Widget* sender);
        void onSelectedBackground(MyGUI::Widget* sender);
        void onMouseWheelMoved(MyGUI::Widget* _sender, int _rel);

        std::unique_ptr<ItemModel> mModel;
        MyGUI::ScrollView* mScrollView;

        int mItemCount = 0;
        int mRows;
        int mControllerFocus = 0;
        bool mControllerActiveWindow;
        void updateControllerFocus(int prevFocus, int newFocus);
    };

}

#endif
