#ifndef OPENMW_WIDGETS_BOX_H
#define OPENMW_WIDGETS_BOX_H

#include <MyGUI_Widget.h>
#include <MyGUI_TextBox.h>
#include <MyGUI_EditBox.h>
#include <MyGUI_Button.h>

namespace Gui
{

    class AutoSizedWidget
    {
    public:
        AutoSizedWidget() : mExpandDirection(MyGUI::Align::Right) {}

        virtual MyGUI::IntSize getRequestedSize() = 0;

    protected:
        void notifySizeChange(MyGUI::Widget* w);

        MyGUI::Align mExpandDirection;
    };

    class AutoSizedTextBox : public AutoSizedWidget, public MyGUI::TextBox
    {
        MYGUI_RTTI_DERIVED( AutoSizedTextBox )

    public:
        virtual MyGUI::IntSize getRequestedSize();
        virtual void setCaption(const MyGUI::UString& _value);

    protected:
        virtual void setPropertyOverride(const std::string& _key, const std::string& _value);
    };

    class AutoSizedEditBox : public AutoSizedWidget, public MyGUI::EditBox
    {
        MYGUI_RTTI_DERIVED( AutoSizedEditBox )

    public:

        virtual MyGUI::IntSize getRequestedSize();
        virtual void setCaption(const MyGUI::UString& _value);

        virtual void initialiseOverride();

    protected:
        virtual void setPropertyOverride(const std::string& _key, const std::string& _value);
    };

    class AutoSizedButton : public AutoSizedWidget, public MyGUI::Button
    {
        MYGUI_RTTI_DERIVED( AutoSizedButton )

    public:
        virtual MyGUI::IntSize getRequestedSize();
        virtual void setCaption(const MyGUI::UString& _value);

    protected:
        virtual void setPropertyOverride(const std::string& _key, const std::string& _value);
    };

    /**
     * @brief A container widget that automatically sizes its children
     * @note the box being an AutoSizedWidget as well allows to put boxes inside a box
     */
    class Box : public AutoSizedWidget
    {
    public:
        Box();

        void notifyChildrenSizeChanged();

    protected:
        virtual void align() = 0;

        virtual bool _setPropertyImpl(const std::string& _key, const std::string& _value);

        int mSpacing; // how much space to put between elements

        int mPadding; // outer padding

        bool mAutoResize; // auto resize the box so that it exactly fits all elements
    };

    class Spacer : public AutoSizedWidget, public MyGUI::Widget
    {
        MYGUI_RTTI_DERIVED( Spacer )
    public:
        Spacer();

        virtual MyGUI::IntSize getRequestedSize() { return MyGUI::IntSize(0,0); }
    };

    class HBox : public Box, public MyGUI::Widget
    {
        MYGUI_RTTI_DERIVED( HBox )

    public:
        virtual void setSize (const MyGUI::IntSize &_value);
        virtual void setCoord (const MyGUI::IntCoord &_value);

    protected:
        virtual void initialiseOverride();

        virtual void align();
        virtual MyGUI::IntSize getRequestedSize();

        virtual void setPropertyOverride(const std::string& _key, const std::string& _value);

        virtual void onWidgetCreated(MyGUI::Widget* _widget);
    };

    class VBox : public Box, public MyGUI::Widget
    {
        MYGUI_RTTI_DERIVED( VBox)

    public:
        virtual void setSize (const MyGUI::IntSize &_value);
        virtual void setCoord (const MyGUI::IntCoord &_value);

    protected:
        virtual void initialiseOverride();

        virtual void align();
        virtual MyGUI::IntSize getRequestedSize();

        virtual void setPropertyOverride(const std::string& _key, const std::string& _value);

        virtual void onWidgetCreated(MyGUI::Widget* _widget);
    };

}

#endif
