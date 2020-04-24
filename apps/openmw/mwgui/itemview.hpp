#ifndef MWGUI_ITEMVIEW_H
#define MWGUI_ITEMVIEW_H

#include <MyGUI_Widget.h>

#include "itemmodel.hpp"

namespace MWGui
{

    class ItemView final : public MyGUI::Widget
    {
    MYGUI_RTTI_DERIVED(ItemView)
    public:
        ItemView();
        virtual ~ItemView();

        /// Register needed components with MyGUI's factory manager
        static void registerComponents ();

        /// Takes ownership of \a model
        void setModel (ItemModel* model);

        typedef MyGUI::delegates::CMultiDelegate1<ItemModel::ModelIndex> EventHandle_ModelIndex;
        typedef MyGUI::delegates::CMultiDelegate0 EventHandle_Void;
        /// Fired when an item was clicked
        EventHandle_ModelIndex eventItemClicked;
        /// Fired when the background was clicked (useful for drag and drop)
        EventHandle_Void eventBackgroundClicked;

        void update();

        void resetScrollBars();

    private:
        void initialiseOverride() final;

        void layoutWidgets();

        void setSize(const MyGUI::IntSize& _value) final;
        void setCoord(const MyGUI::IntCoord& _value) final;

        void onSelectedItem (MyGUI::Widget* sender);
        void onSelectedBackground (MyGUI::Widget* sender);
        void onMouseWheelMoved(MyGUI::Widget* _sender, int _rel);

        ItemModel* mModel;
        MyGUI::ScrollView* mScrollView;

    };

}

#endif
