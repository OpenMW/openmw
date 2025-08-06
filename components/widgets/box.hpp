#ifndef OPENMW_WIDGETS_BOX_H
#define OPENMW_WIDGETS_BOX_H

#include <MyGUI_Button.h>
#include <MyGUI_ComboBox.h>
#include <MyGUI_EditBox.h>
#include <MyGUI_TextBox.h>
#include <MyGUI_Widget.h>

#include "fontwrapper.hpp"

namespace Gui
{
    class Button : public FontWrapper<MyGUI::Button>
    {
        MYGUI_RTTI_DERIVED(Button)
    };

    class TextBox : public FontWrapper<MyGUI::TextBox>
    {
        MYGUI_RTTI_DERIVED(TextBox)
    };

    class EditBox : public FontWrapper<MyGUI::EditBox>
    {
        MYGUI_RTTI_DERIVED(EditBox)
    };

    class AutoSizedWidget
    {
    public:
        AutoSizedWidget()
            : mExpandDirection(MyGUI::Align::Right)
        {
        }

        virtual MyGUI::IntSize getRequestedSize() = 0;

        virtual ~AutoSizedWidget() = default;

    protected:
        void notifySizeChange(MyGUI::Widget* w);

        MyGUI::Align mExpandDirection;
    };

    class AutoSizedTextBox : public AutoSizedWidget, public TextBox
    {
        MYGUI_RTTI_DERIVED(AutoSizedTextBox)

    public:
        MyGUI::IntSize getRequestedSize() override;
        void setCaption(const MyGUI::UString& value) override;

    protected:
        void setPropertyOverride(std::string_view key, std::string_view value) override;
        std::string mFontSize;
    };

    class AutoSizedEditBox : public AutoSizedWidget, public EditBox
    {
        MYGUI_RTTI_DERIVED(AutoSizedEditBox)

    public:
        MyGUI::IntSize getRequestedSize() override;
        void setCaption(const MyGUI::UString& value) override;

        void initialiseOverride() override;

    protected:
        void setPropertyOverride(std::string_view key, std::string_view value) override;
        int getWidth();
        std::string mFontSize;
        bool mShrink = false;
        bool mWasResized = false;
        int mMaxWidth = 0;
    };

    class AutoSizedButton : public AutoSizedWidget, public Button
    {
        MYGUI_RTTI_DERIVED(AutoSizedButton)

    public:
        MyGUI::IntSize getRequestedSize() override;
        void setCaption(const MyGUI::UString& value) override;

    protected:
        void setPropertyOverride(std::string_view key, std::string_view value) override;
        std::string mFontSize;
    };

    /**
     * @brief A container widget that automatically sizes its children
     * @note the box being an AutoSizedWidget as well allows to put boxes inside a box
     */
    class Box : public AutoSizedWidget
    {
    public:
        Box();

        virtual ~Box() = default;

        void notifyChildrenSizeChanged();

    protected:
        virtual void align() = 0;

        virtual bool _setPropertyImpl(std::string_view key, std::string_view value);

        int mSpacing; // how much space to put between elements

        int mPadding; // outer padding

        bool mAutoResize; // auto resize the box so that it exactly fits all elements
    };

    class Spacer : public AutoSizedWidget, public MyGUI::Widget
    {
        MYGUI_RTTI_DERIVED(Spacer)
    public:
        Spacer();

        MyGUI::IntSize getRequestedSize() override { return MyGUI::IntSize(0, 0); }
    };

    class HBox : public Box, public MyGUI::Widget
    {
        MYGUI_RTTI_DERIVED(HBox)

    public:
        void setSize(const MyGUI::IntSize& value) override;
        void setCoord(const MyGUI::IntCoord& value) override;

    protected:
        void initialiseOverride() override;

        void align() override;
        MyGUI::IntSize getRequestedSize() override;

        void setPropertyOverride(std::string_view key, std::string_view value) override;

        void onWidgetCreated(MyGUI::Widget* widget) override;
    };

    class VBox : public Box, public MyGUI::Widget
    {
        MYGUI_RTTI_DERIVED(VBox)

    public:
        void setSize(const MyGUI::IntSize& value) override;
        void setCoord(const MyGUI::IntCoord& value) override;

    protected:
        void initialiseOverride() override;

        void align() override;
        MyGUI::IntSize getRequestedSize() override;

        void setPropertyOverride(std::string_view key, std::string_view value) override;

        void onWidgetCreated(MyGUI::Widget* widget) override;
    };

}

#endif
