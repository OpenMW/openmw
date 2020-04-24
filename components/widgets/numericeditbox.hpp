#ifndef OPENMW_NUMERIC_EDIT_BOX_H
#define OPENMW_NUMERIC_EDIT_BOX_H

#include <MyGUI_EditBox.h>

#include "fontwrapper.hpp"

namespace Gui
{

    /**
     * @brief A variant of the EditBox that only allows integer inputs
     */
    class NumericEditBox final : public FontWrapper<MyGUI::EditBox>
    {
        MYGUI_RTTI_DERIVED(NumericEditBox)

    public:
        NumericEditBox()
            : mValue(0), mMinValue(std::numeric_limits<int>::min()),
            mMaxValue(std::numeric_limits<int>::max())
        {
        }

        void initialiseOverride() final;
        void shutdownOverride() final;

        typedef MyGUI::delegates::CMultiDelegate1<int> EventHandle_ValueChanged;
        EventHandle_ValueChanged eventValueChanged;

        /// @note Does not trigger eventValueChanged
        void setValue (int value);
        int getValue();

        void setMinValue(int minValue);
        void setMaxValue(int maxValue);
    private:
        void onEditTextChange(MyGUI::EditBox* sender);
        void onKeyLostFocus(MyGUI::Widget* _new) final;
        void onKeyButtonPressed(MyGUI::KeyCode key, MyGUI::Char character) final;

        int mValue;

        int mMinValue;
        int mMaxValue;
    };

}

#endif
