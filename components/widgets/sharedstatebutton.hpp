#ifndef OPENMW_WIDGETS_SHAREDSTATEBUTTON_HPP
#define OPENMW_WIDGETS_SHAREDSTATEBUTTON_HPP

#include <MyGUI_Button.h>

#include "fontwrapper.hpp"

namespace Gui
{

    class SharedStateButton;

    typedef std::vector<SharedStateButton*> ButtonGroup;

    /// @brief A button that applies its own state changes to other widgets, to do this you define it as part of a ButtonGroup.
    class SharedStateButton final : public FontWrapper<MyGUI::Button>
    {
    MYGUI_RTTI_DERIVED(SharedStateButton)

    public:
        SharedStateButton();

    protected:
        void updateButtonState();

        void onMouseButtonPressed(int _left, int _top, MyGUI::MouseButton _id) final;
        void onMouseButtonReleased(int _left, int _top, MyGUI::MouseButton _id) final;
        void onMouseSetFocus(MyGUI::Widget* _old) final;
        void onMouseLostFocus(MyGUI::Widget* _new) final;
        void baseUpdateEnable() final;

        void shutdownOverride() final;

        bool _setState(const std::string &_value);

    public:
        void shareStateWith(ButtonGroup shared);

        /// @note The ButtonGroup connection will be destroyed when any widget in the group gets destroyed.
        static void createButtonGroup(ButtonGroup group);

        //! Set button selected state
        void setStateSelected(bool _value);

    private:
        ButtonGroup mSharedWith;

        bool mIsMousePressed;
        bool mIsMouseFocus;
    };
}

#endif
