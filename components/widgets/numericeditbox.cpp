#include "numericeditbox.hpp"

#include <boost/lexical_cast.hpp>

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
            mValue = boost::lexical_cast<int>(newCaption);
            int capped = std::min(mMaxValue, std::max(mValue, mMinValue));
            if (capped != mValue)
            {
                mValue = capped;
                setCaption(MyGUI::utility::toString(mValue));
            }
        }
        catch (boost::bad_lexical_cast&)
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

}
