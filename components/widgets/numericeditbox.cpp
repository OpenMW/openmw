#include <stdexcept>

#include "numericeditbox.hpp"

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

    void NumericEditBox::onEditTextChange(MyGUI::EditBox *sender)
    {
        std::string newCaption = sender->getCaption();
        if (newCaption.empty())
        {
            return;
        }

        try
        {
            mValue = std::stoi(newCaption);
            int capped = std::min(mMaxValue, std::max(mValue, mMinValue));
            if (capped != mValue)
            {
                mValue = capped;
                setCaption(MyGUI::utility::toString(mValue));
            }
        }
        catch (const std::invalid_argument&)
        {
            setCaption(MyGUI::utility::toString(mValue));
        }
        catch (const std::out_of_range&)
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

    void NumericEditBox::onKeyLostFocus(MyGUI::Widget* _new)
    {
        Base::onKeyLostFocus(_new);
        setCaption(MyGUI::utility::toString(mValue));
    }

    void NumericEditBox::onKeyButtonPressed(MyGUI::KeyCode key, MyGUI::Char character)
    {
        if (key == MyGUI::KeyCode::ArrowUp)
        {
            setValue(std::min(mValue+1, mMaxValue));
            eventValueChanged(mValue);
        }
        else if (key == MyGUI::KeyCode::ArrowDown)
        {
            setValue(std::max(mValue-1, mMinValue));
            eventValueChanged(mValue);
        }
        else if (character == 0 || (character >= '0' && character <= '9'))
            Base::onKeyButtonPressed(key, character);
    }

}
