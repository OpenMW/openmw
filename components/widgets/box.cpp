#include "box.hpp"

#include <MyGUI_EditText.h>

namespace Gui
{

    void AutoSizedWidget::notifySizeChange (MyGUI::Widget* w)
    {
        MyGUI::Widget * parent = w->getParent();
        if (parent != 0)
        {
            if (mExpandDirection.isLeft())
            {
                int hdiff = getRequestedSize ().width - w->getSize().width;
                w->setPosition(w->getPosition() - MyGUI::IntPoint(hdiff, 0));
            }
            w->setSize(getRequestedSize ());

            while (parent != 0)
            {
                Box * b = dynamic_cast<Box*>(parent);
                if (b)
                    b->notifyChildrenSizeChanged();
                else
                    break;
                parent = parent->getParent();
            }
        }
    }


    MyGUI::IntSize AutoSizedTextBox::getRequestedSize()
    {
        return getTextSize();
    }

    void AutoSizedTextBox::setCaption(const MyGUI::UString& _value)
    {
        TextBox::setCaption(_value);

        notifySizeChange (this);
    }

    void AutoSizedTextBox::setPropertyOverride(const std::string& _key, const std::string& _value)
    {
        if (_key == "ExpandDirection")
        {
            mExpandDirection = MyGUI::Align::parse (_value);
        }
        else
        {
            Gui::TextBox::setPropertyOverride (_key, _value);
        }
    }

    int AutoSizedEditBox::getWidth()
    {
        // If the widget has the one short text line, we can shrink widget to avoid a lot of empty space.
        int textWidth = mMaxWidth;

        if (mShrink)
        {
            // MyGUI needs to know the widget size for wordwrapping, but we will know the widget size only after wordwrapping.
            // To solve this issue, use the maximum tooltip width first for wordwrapping, then resize widget to its actual used space.
            if (mWasResized)
            {
                int maxLineWidth = 0;
                const MyGUI::VectorLineInfo & lines = getSubWidgetText()->castType<MyGUI::EditText>()->getLineInfo();
                for (unsigned int i = 0; i < lines.size(); ++i)
                    maxLineWidth = std::max(maxLineWidth, lines[i].width);

                textWidth = std::min(maxLineWidth, textWidth);
            }
            else
            {
                mWasResized = true;
            }
        }

        return textWidth;
    }

    MyGUI::IntSize AutoSizedEditBox::getRequestedSize()
    {
        if (getAlign().isHStretch())
            throw std::runtime_error("AutoSizedEditBox can't have HStretch align (" + getName() + ")");
        return MyGUI::IntSize(getWidth(), getTextSize().height);
    }

    void AutoSizedEditBox::setCaption(const MyGUI::UString& _value)
    {
        EditBox::setCaption(_value);
        mWasResized = false;

        notifySizeChange (this);
    }

    void AutoSizedEditBox::initialiseOverride()
    {
        mMaxWidth = getSize().width;
        Base::initialiseOverride();
        setNeedKeyFocus(false);
        setEditStatic(true);
    }

    void AutoSizedEditBox::setPropertyOverride(const std::string& _key, const std::string& _value)
    {
        if (_key == "ExpandDirection")
        {
            mExpandDirection = MyGUI::Align::parse (_value);
        }
        else if (_key == "Shrink")
        {
            mShrink = MyGUI::utility::parseValue<bool>(_value);
        }
        else
        {
            Gui::EditBox::setPropertyOverride (_key, _value);
        }
    }

    MyGUI::IntSize AutoSizedButton::getRequestedSize()
    {
        MyGUI::IntSize padding(24, 8);
        if (isUserString("TextPadding"))
            padding = MyGUI::IntSize::parse(getUserString("TextPadding"));

        MyGUI::IntSize size = getTextSize() + MyGUI::IntSize(padding.width,padding.height);
        return size;
    }

    void AutoSizedButton::setCaption(const MyGUI::UString& _value)
    {
        Button::setCaption(_value);

        notifySizeChange (this);
    }

    void AutoSizedButton::setPropertyOverride(const std::string& _key, const std::string& _value)
    {
        if (_key == "ExpandDirection")
        {
            mExpandDirection = MyGUI::Align::parse (_value);
        }
        else
        {
            Gui::Button::setPropertyOverride (_key, _value);
        }
    }
    Box::Box()
        : mSpacing(4)
        , mPadding(0)
        , mAutoResize(false)
    {
    }

    void Box::notifyChildrenSizeChanged ()
    {
        align();
    }

    bool Box::_setPropertyImpl(const std::string& _key, const std::string& _value)
    {
        if (_key == "Spacing")
            mSpacing = MyGUI::utility::parseValue<int>(_value);
        else if (_key == "Padding")
            mPadding = MyGUI::utility::parseValue<int>(_value);
        else if (_key == "AutoResize")
            mAutoResize = MyGUI::utility::parseValue<bool>(_value);
        else
            return false;

        return true;
    }

    void HBox::align ()
    {
        unsigned int count = getChildCount ();
        size_t h_stretched_count = 0;
        int total_width = 0;
        int total_height = 0;
        std::vector< std::pair<MyGUI::IntSize, bool> > sizes;
        sizes.resize(count);

        for (unsigned int i = 0; i < count; ++i)
        {
            MyGUI::Widget* w = getChildAt(i);
            bool hstretch = w->getUserString ("HStretch") == "true";
            bool hidden = w->getUserString("Hidden") == "true";
            if (hidden)
                continue;
            h_stretched_count += hstretch;
            AutoSizedWidget* aw = dynamic_cast<AutoSizedWidget*>(w);
            if (aw)
            {
                sizes[i] = std::make_pair(aw->getRequestedSize (), hstretch);
                total_width += aw->getRequestedSize ().width;
                total_height = std::max(total_height, aw->getRequestedSize ().height);
            }
            else
            {
                sizes[i] = std::make_pair(w->getSize(), hstretch);
                total_width += w->getSize().width;
                if (!(w->getUserString("VStretch") == "true"))
                    total_height = std::max(total_height, w->getSize().height);
            }

            if (i != count-1)
                total_width += mSpacing;
        }

        if (mAutoResize && (total_width+mPadding*2 != getClientCoord().width || total_height+mPadding*2 != getClientCoord().height))
        {
            int xmargin = getSize().width - getClientCoord().width;
            int ymargin = getSize().height - getClientCoord().height;
            setSize(MyGUI::IntSize(total_width+mPadding*2 + xmargin, total_height+mPadding*2 + ymargin));
            return;
        }


        int curX = 0;
        for (unsigned int i = 0; i < count; ++i)
        {
            if (i == 0)
                curX += mPadding;

            MyGUI::Widget* w = getChildAt(i);

            bool hidden = w->getUserString("Hidden") == "true";
            if (hidden)
                continue;

            bool vstretch = w->getUserString ("VStretch") == "true";
            int max_height = getClientCoord().height - mPadding*2;
            int height = vstretch ? max_height : sizes[i].first.height;

            MyGUI::IntCoord widgetCoord;
            widgetCoord.left = curX;
            widgetCoord.top = mPadding + (getClientCoord().height-mPadding*2 - height) / 2;

            int width = 0;
            if (sizes[i].second)
            {
                if (h_stretched_count == 0)
                    throw std::logic_error("unexpected");
                width = sizes[i].first.width + (getClientCoord().width-mPadding*2 - total_width)/h_stretched_count;
            }
            else
                width = sizes[i].first.width;

            widgetCoord.width = width;
            widgetCoord.height = height;
            w->setCoord(widgetCoord);
            curX += width;

            if (i != count-1)
                curX += mSpacing;
        }
    }

    void HBox::setPropertyOverride(const std::string& _key, const std::string& _value)
    {
        if (!Box::_setPropertyImpl (_key, _value))
            MyGUI::Widget::setPropertyOverride(_key, _value);
    }

    void HBox::setSize (const MyGUI::IntSize& _value)
    {
        MyGUI::Widget::setSize (_value);
        align();
    }

    void HBox::setCoord (const MyGUI::IntCoord& _value)
    {
        MyGUI::Widget::setCoord (_value);
        align();
    }

    void HBox::initialiseOverride()
    {
        Base::initialiseOverride();
        MyGUI::Widget* client = 0;
        assignWidget(client, "Client");
        setWidgetClient(client);
    }

    void HBox::onWidgetCreated(MyGUI::Widget* _widget)
    {
        align();
    }

    MyGUI::IntSize HBox::getRequestedSize ()
    {
        MyGUI::IntSize size(0,0);
        for (unsigned int i = 0; i < getChildCount (); ++i)
        {
            bool hidden = getChildAt(i)->getUserString("Hidden") == "true";
            if (hidden)
                continue;

            AutoSizedWidget* w = dynamic_cast<AutoSizedWidget*>(getChildAt(i));
            if (w)
            {
                MyGUI::IntSize requested = w->getRequestedSize ();
                size.height = std::max(size.height, requested.height);
                size.width = size.width + requested.width;
                if (i != getChildCount()-1)
                    size.width += mSpacing;
            }
            else
            {
                MyGUI::IntSize requested = getChildAt(i)->getSize ();
                size.height = std::max(size.height, requested.height);

                if (getChildAt(i)->getUserString("HStretch") != "true")
                    size.width = size.width + requested.width;

                if (i != getChildCount()-1)
                    size.width += mSpacing;
            }
            size.height += mPadding*2;
            size.width += mPadding*2;
        }
        return size;
    }




    void VBox::align ()
    {
        unsigned int count = getChildCount ();
        size_t v_stretched_count = 0;
        int total_height = 0;
        int total_width = 0;
        std::vector< std::pair<MyGUI::IntSize, bool> > sizes;
        sizes.resize(count);
        for (unsigned int i = 0; i < count; ++i)
        {
            MyGUI::Widget* w = getChildAt(i);

            bool hidden = w->getUserString("Hidden") == "true";
            if (hidden)
                continue;

            bool vstretch = w->getUserString ("VStretch") == "true";
            v_stretched_count += vstretch;
            AutoSizedWidget* aw = dynamic_cast<AutoSizedWidget*>(w);
            if (aw)
            {
                sizes[i] = std::make_pair(aw->getRequestedSize (), vstretch);
                total_height += aw->getRequestedSize ().height;
                total_width = std::max(total_width, aw->getRequestedSize ().width);
            }
            else
            {
                sizes[i] = std::make_pair(w->getSize(), vstretch);
                total_height += w->getSize().height;

                if (!(w->getUserString("HStretch") == "true"))
                    total_width = std::max(total_width, w->getSize().width);
            }

            if (i != count-1)
                total_height += mSpacing;
        }

        if (mAutoResize && (total_width+mPadding*2 != getClientCoord().width || total_height+mPadding*2 != getClientCoord().height))
        {
            int xmargin = getSize().width - getClientCoord().width;
            int ymargin = getSize().height - getClientCoord().height;
            setSize(MyGUI::IntSize(total_width+mPadding*2 + xmargin, total_height+mPadding*2 + ymargin));
            return;
        }

        int curY = 0;
        for (unsigned int i = 0; i < count; ++i)
        {
            if (i==0)
                curY += mPadding;

            MyGUI::Widget* w = getChildAt(i);

            bool hidden = w->getUserString("Hidden") == "true";
            if (hidden)
                continue;

            bool hstretch = w->getUserString ("HStretch") == "true";
            int maxWidth = getClientCoord().width - mPadding*2;
            int width = hstretch ? maxWidth : sizes[i].first.width;

            MyGUI::IntCoord widgetCoord;
            widgetCoord.top = curY;
            widgetCoord.left = mPadding + (getClientCoord().width-mPadding*2 - width) / 2;

            int height = 0;
            if (sizes[i].second)
            {
                if (v_stretched_count == 0)
                    throw std::logic_error("unexpected");
                height = sizes[i].first.height + (getClientCoord().height-mPadding*2 - total_height)/v_stretched_count;
            }
            else
                height = sizes[i].first.height;

            widgetCoord.height = height;
            widgetCoord.width = width;
            w->setCoord(widgetCoord);
            curY += height;

            if (i != count-1)
                curY += mSpacing;
        }
    }

    void VBox::setPropertyOverride(const std::string& _key, const std::string& _value)
    {
        if (!Box::_setPropertyImpl (_key, _value))
            MyGUI::Widget::setPropertyOverride(_key, _value);
    }

    void VBox::setSize (const MyGUI::IntSize& _value)
    {
        MyGUI::Widget::setSize (_value);
        align();
    }

    void VBox::setCoord (const MyGUI::IntCoord& _value)
    {
        MyGUI::Widget::setCoord (_value);
        align();
    }

    void VBox::initialiseOverride()
    {
        Base::initialiseOverride();
        MyGUI::Widget* client = 0;
        assignWidget(client, "Client");
        setWidgetClient(client);
    }

    MyGUI::IntSize VBox::getRequestedSize ()
    {
        MyGUI::IntSize size(0,0);
        for (unsigned int i = 0; i < getChildCount (); ++i)
        {
            bool hidden = getChildAt(i)->getUserString("Hidden") == "true";
            if (hidden)
                continue;

            AutoSizedWidget* w = dynamic_cast<AutoSizedWidget*>(getChildAt(i));
            if (w)
            {
                MyGUI::IntSize requested = w->getRequestedSize ();
                size.width = std::max(size.width, requested.width);
                size.height = size.height + requested.height;
                if (i != getChildCount()-1)
                    size.height += mSpacing;
            }
            else
            {
                MyGUI::IntSize requested = getChildAt(i)->getSize ();
                size.width = std::max(size.width, requested.width);

                if (getChildAt(i)->getUserString("VStretch") != "true")
                    size.height = size.height + requested.height;

                if (i != getChildCount()-1)
                    size.height += mSpacing;
            }
            size.height += mPadding*2;
            size.width += mPadding*2;
        }
        return size;
    }

    void VBox::onWidgetCreated(MyGUI::Widget* _widget)
    {
        align();
    }

    Spacer::Spacer()
    {
        setUserString("HStretch", "true");
        setUserString("VStretch", "true");
    }

}
