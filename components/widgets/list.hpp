#ifndef OPENMW_COMPONENTS_WIDGETS_LIST_HPP
#define OPENMW_COMPONENTS_WIDGETS_LIST_HPP

#include <MyGUI_ScrollView.h>

namespace Gui
{
    /**
     * \brief a very simple list widget that supports word-wrapping entries
     * \note if the width or height of the list changes, you must call adjustSize() method
     */
    class MWList : public MyGUI::Widget
    {
        MYGUI_RTTI_DERIVED(MWList)
    public:
        MWList();

        typedef MyGUI::delegates::CMultiDelegate2<const std::string&, int> EventHandle_StringInt;
        typedef MyGUI::delegates::CMultiDelegate1<MyGUI::Widget*> EventHandle_Widget;

        /**
         * Event: Item selected with the mouse.
         * signature: void method(std::string itemName, int index)
         */
        EventHandle_StringInt eventItemSelected;

        /**
         * Event: Item selected with the mouse.
         * signature: void method(MyGUI::Widget* sender)
         */
        EventHandle_Widget eventWidgetSelected;


        /**
         * Call after the size of the list changed, or items were inserted/removed
         */
        void adjustSize();

        void addItem(const std::string& name);
        void addSeparator(); ///< add a seperator between the current and the next item.
        void removeItem(const std::string& name);
        unsigned int getItemCount();
        std::string getItemNameAt(unsigned int at); ///< \attention if there are separators, this method will return "" at the place where the separator is
        void clear();

        MyGUI::Button* getItemWidget(const std::string& name);
        ///< get widget for an item name, useful to set up tooltip

        void scrollToTop();

        void setPropertyOverride(const std::string& _key, const std::string& _value) final;

    protected:
        void initialiseOverride() final;

        void redraw(bool scrollbarShown = false);

        void onMouseWheelMoved(MyGUI::Widget* _sender, int _rel);
        void onItemSelected(MyGUI::Widget* _sender);

    private:
        MyGUI::ScrollView* mScrollView;
        MyGUI::Widget* mClient;
        std::string mListItemSkin;

        std::vector<std::string> mItems;

        int mItemHeight; // height of all items
    };
}

#endif
