#ifndef OPENMW_NUMERIC_EDIT_BOX_H
#define OPENMW_NUMERIC_EDIT_BOX_H

#include <MyGUI_EditBox.h>

namespace Gui
{

    /**
     * @brief A variant of the EditBox that only allows integer inputs
     */
    class NumericEditBox : public MyGUI::EditBox
    {
        MYGUI_RTTI_DERIVED(NumericEditBox)

    public:
        NumericEditBox()
            : mValue(0), mMinValue(std::numeric_limits<int>::min()),
            mMaxValue(std::numeric_limits<int>::max())
        {}

        void initialiseOverride();
        void shutdownOverride();

        typedef MyGUI::delegates::CMultiDelegate1<int> EventHandle_ValueChanged;
        EventHandle_ValueChanged eventValueChanged;

        /// @note Does not trigger eventValueChanged
        void setValue (int value);

        void setMinValue(int minValue);
        void setMaxValue(int maxValue);
    private:
        void onEditTextChange(MyGUI::EditBox* sender);
        void onKeyLostFocus(MyGUI::Widget* _new);

        int mValue;

        int mMinValue;
        int mMaxValue;
    };

}

#endif
