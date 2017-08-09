#ifndef OPENMW_MWGUI_ITEMWIDGET_H
#define OPENMW_MWGUI_ITEMWIDGET_H

#include <MyGUI_Widget.h>

namespace MWWorld
{
    class Ptr;
}

namespace MWGui
{

    /// @brief A widget that shows an icon for an MWWorld::Ptr
    class ItemWidget : public MyGUI::Widget
    {
    MYGUI_RTTI_DERIVED(ItemWidget)
    public:
        ItemWidget();

        /// Register needed components with MyGUI's factory manager
        static void registerComponents ();

        enum ItemState
        {
            None,
            Equip,
            Barter,
            Magic
        };

        /// Set count to be displayed in a textbox over the item
        void setCount(int count);

        /// \a ptr may be empty
        void setItem (const MWWorld::Ptr& ptr, ItemState state = None);

        // Set icon and frame manually
        void setIcon (const std::string& icon);
        void setIcon (const MWWorld::Ptr& ptr);
        void setFrame (const std::string& frame, const MyGUI::IntCoord& coord);

    private:
        virtual void initialiseOverride();

        MyGUI::ImageBox* mItem;
        MyGUI::ImageBox* mItemShadow;
        MyGUI::ImageBox* mFrame;
        MyGUI::TextBox* mText;

        std::string mCurrentIcon;
        std::string mCurrentFrame;
    };

}

#endif
