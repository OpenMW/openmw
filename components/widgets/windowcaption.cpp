#include "windowcaption.hpp"

#include <stdexcept>

namespace Gui
{

    WindowCaption::WindowCaption()
        : mLeft(nullptr)
        , mRight(nullptr)
        , mClient(nullptr)
    {
    }

    void WindowCaption::initialiseOverride()
    {
        Base::initialiseOverride();

        assignWidget(mLeft, "Left");
        assignWidget(mRight, "Right");

        assignWidget(mClient, "Client");
        if (!mClient)
            throw std::runtime_error("WindowCaption needs an EditBox Client widget in its skin");
    }

    void WindowCaption::setCaption(const MyGUI::UString &_value)
    {
        EditBox::setCaption(_value);
        align();
    }

    void WindowCaption::setSize(const MyGUI::IntSize& _value)
    {
        Base::setSize(_value);
        align();
    }

    void WindowCaption::setCoord(const MyGUI::IntCoord& _value)
    {
        Base::setCoord(_value);
        align();
    }

    void WindowCaption::align()
    {
        MyGUI::IntSize textSize = getTextSize();
        MyGUI::Widget* caption = mClient;
        caption->setSize(textSize.width + 24, caption->getHeight());

        int barwidth = (getWidth()-caption->getWidth())/2;
        caption->setPosition(barwidth, caption->getTop());
        if (mLeft)
            mLeft->setCoord(0, mLeft->getTop(), barwidth, mLeft->getHeight());
        if (mRight)
            mRight->setCoord(barwidth + caption->getWidth(), mRight->getTop(), barwidth, mRight->getHeight());
    }

}
