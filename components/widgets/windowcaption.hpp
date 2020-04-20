#ifndef OPENMW_WIDGETS_WINDOWCAPTION_H
#define OPENMW_WIDGETS_WINDOWCAPTION_H

#include <MyGUI_EditBox.h>

namespace Gui
{

    /// Window caption that automatically adjusts "Left" and "Right" widgets in its skin
    /// based on the text size of the caption in the middle
    class WindowCaption final : public MyGUI::EditBox
    {
        MYGUI_RTTI_DERIVED(WindowCaption)
    public:
        WindowCaption();

        void setCaption(const MyGUI::UString &_value) final;
        void initialiseOverride() final;

        void setSize(const MyGUI::IntSize& _value) final;
        void setCoord(const MyGUI::IntCoord& _value) final;

    private:
        MyGUI::Widget* mLeft;
        MyGUI::Widget* mRight;
        MyGUI::Widget* mClient;

        void align();
    };

}

#endif
