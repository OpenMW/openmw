#ifndef OPENMW_WIDGETS_SHAREDSTATEBUTTON_HPP
#define OPENMW_WIDGETS_SHAREDSTATEBUTTON_HPP

#include <MyGUI_Button.h>

#include "fontwrapper.hpp"

namespace Gui
{

    class SharedStateButton;

    typedef std::vector<SharedStateButton*> ButtonGroup;

    /// @brief A button that applies its own state changes to other widgets, to do this you define it as part of a
    /// ButtonGroup.
    class SharedStateButton final : public FontWrapper<MyGUI::Button>
    {
        MYGUI_RTTI_DERIVED(SharedStateButton)

    public:
        SharedStateButton();

        void onMouseSetFocus(MyGUI::Widget* oldWidget) override;
        void onMouseLostFocus(MyGUI::Widget* newWidget) override;

    protected:
        void updateButtonState();

        void onMouseButtonPressed(int left, int top, MyGUI::MouseButton id) override;
        void onMouseButtonReleased(int left, int top, MyGUI::MouseButton id) override;
        void baseUpdateEnable() override;

        void shutdownOverride() override;

        bool _setState(const std::string& value);

    public:
        void shareStateWith(const ButtonGroup& shared);

        /// @note The ButtonGroup connection will be destroyed when any widget in the group gets destroyed.
        static void createButtonGroup(ButtonGroup& group);

        //! Set button selected state
        void setStateSelected(bool value);

    private:
        ButtonGroup mSharedWith;

        bool mIsMousePressed;
        bool mIsMouseFocus;
    };
}

#endif
