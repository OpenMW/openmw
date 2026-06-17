#include <stdexcept>

#include "numericeditbox.hpp"

#include <components/misc/strings/conversion.hpp>

namespace Gui
{

    void NumericEditBox::initialiseOverride()
    {
        Base::initialiseOverride();
        eventEditTextChange += MyGUI::newDelegate(this, &NumericEditBox::onEditTextChange);

        mValue = 0;
        setCaption("0");
    }

    void NumericEditBox::shutdownOverride()
    {
        Base::shutdownOverride();
        eventEditTextChange -= MyGUI::newDelegate(this, &NumericEditBox::onEditTextChange);
    }

    void NumericEditBox::onEditTextChange(MyGUI::EditBox* sender)
    {
        const std::string newCaption = sender->getCaption();

        if (newCaption.empty())
        {
            return;
        }

        if (const auto conversion = Misc::StringUtils::toNumeric<int>(newCaption))
        {
            mValue = conversion.value();

            const int capped = std::clamp(mValue, mMinValue, mMaxValue);
            if (capped != mValue)
            {
                mValue = capped;
                setCaption(MyGUI::utility::toString(mValue));
            }
        }
        else
        {
            setCaption(MyGUI::utility::toString(mValue));
        }

        eventValueChanged(mValue);
    }

    void NumericEditBox::setValue(int value)
    {
        if (value != mValue)
        {
            setCaption(MyGUI::utility::toString(value));
            mValue = value;
        }
    }

    int NumericEditBox::getValue()
    {
        return mValue;
    }

    void NumericEditBox::setMinValue(int minValue)
    {
        mMinValue = minValue;
    }

    void NumericEditBox::setMaxValue(int maxValue)
    {
        mMaxValue = maxValue;
    }

    void NumericEditBox::onKeyLostFocus(MyGUI::Widget* newWidget)
    {
        Base::onKeyLostFocus(newWidget);
        setCaption(MyGUI::utility::toString(mValue));
    }

    void NumericEditBox::onKeyButtonPressed(MyGUI::KeyCode key, MyGUI::Char character)
    {
        if (key == MyGUI::KeyCode::ArrowUp)
        {
            setValue(std::min(mValue + 1, mMaxValue));
            eventValueChanged(mValue);
        }
        else if (key == MyGUI::KeyCode::ArrowDown)
        {
            setValue(std::max(mValue - 1, mMinValue));
            eventValueChanged(mValue);
        }
        else if (character == 0 || (character >= '0' && character <= '9'))
            Base::onKeyButtonPressed(key, character);
    }

}
