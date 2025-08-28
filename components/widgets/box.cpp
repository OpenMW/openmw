#include "box.hpp"

#include <MyGUI_EditText.h>

namespace Gui
{
    void AutoSizedWidget::notifySizeChange(MyGUI::Widget* w)
    {
        MyGUI::Widget* parent = w->getParent();
        if (parent != nullptr)
        {
            if (mExpandDirection.isLeft())
            {
                int hdiff = getRequestedSize().width - w->getSize().width;
                w->setPosition(w->getPosition() - MyGUI::IntPoint(hdiff, 0));
            }
            w->setSize(getRequestedSize());

            while (parent != nullptr)
            {
                Box* b = dynamic_cast<Box*>(parent);
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
        return getCaption().empty() ? MyGUI::IntSize{ 0, 0 } : getTextSize();
    }

    void AutoSizedTextBox::setCaption(const MyGUI::UString& value)
    {
        TextBox::setCaption(value);

        notifySizeChange(this);
    }

    void AutoSizedTextBox::setPropertyOverride(std::string_view key, std::string_view value)
    {
        if (key == "ExpandDirection")
        {
            mExpandDirection = MyGUI::Align::parse(value);
        }
        else
        {
            Gui::TextBox::setPropertyOverride(key, value);
        }
    }

    int AutoSizedEditBox::getWidth()
    {
        // If the widget has the one short text line, we can shrink widget to avoid a lot of empty space.
        int textWidth = mMaxWidth;

        if (mShrink)
        {
            // MyGUI needs to know the widget size for wordwrapping, but we will know the widget size only after
            // wordwrapping. To solve this issue, use the maximum tooltip width first for wordwrapping, then resize
            // widget to its actual used space.
            if (mWasResized)
            {
                int maxLineWidth = 0;
                const MyGUI::VectorLineInfo& lines = getSubWidgetText()->castType<MyGUI::EditText>()->getLineInfo();
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

    void AutoSizedEditBox::setCaption(const MyGUI::UString& value)
    {
        EditBox::setCaption(value);
        mWasResized = false;

        notifySizeChange(this);
    }

    void AutoSizedEditBox::initialiseOverride()
    {
        mMaxWidth = getSize().width;
        Base::initialiseOverride();
        setNeedKeyFocus(false);
        setEditStatic(true);
    }

    void AutoSizedEditBox::setPropertyOverride(std::string_view key, std::string_view value)
    {
        if (key == "ExpandDirection")
        {
            mExpandDirection = MyGUI::Align::parse(value);
        }
        else if (key == "Shrink")
        {
            mShrink = MyGUI::utility::parseValue<bool>(value);
        }
        else
        {
            Gui::EditBox::setPropertyOverride(key, value);
        }
    }

    MyGUI::IntSize AutoSizedButton::getRequestedSize()
    {
        MyGUI::IntSize padding(24, 8);
        if (isUserString("TextPadding"))
            padding = MyGUI::IntSize::parse(getUserString("TextPadding"));

        MyGUI::IntSize size = getTextSize() + MyGUI::IntSize(padding.width, padding.height);
        return size;
    }

    void AutoSizedButton::setCaption(const MyGUI::UString& value)
    {
        Button::setCaption(value);

        notifySizeChange(this);
    }

    void AutoSizedButton::setPropertyOverride(std::string_view key, std::string_view value)
    {
        if (key == "ExpandDirection")
        {
            mExpandDirection = MyGUI::Align::parse(value);
        }
        else
        {
            Gui::Button::setPropertyOverride(key, value);
        }
    }

    Box::Box()
        : mSpacing(4)
        , mPadding(0)
        , mAutoResize(false)
    {
    }

    void Box::notifyChildrenSizeChanged()
    {
        align();
    }

    bool Box::_setPropertyImpl(std::string_view key, std::string_view value)
    {
        if (key == "Spacing")
            mSpacing = MyGUI::utility::parseValue<int>(value);
        else if (key == "Padding")
            mPadding = MyGUI::utility::parseValue<int>(value);
        else if (key == "AutoResize")
            mAutoResize = MyGUI::utility::parseValue<bool>(value);
        else
            return false;

        return true;
    }

    void HBox::align()
    {
        unsigned int count = getChildCount();
        size_t hStretchedCount = 0;
        int totalWidth = 0;
        int totalHeight = 0;
        std::vector<std::pair<MyGUI::IntSize, bool>> sizes;
        sizes.resize(count);

        for (unsigned int i = 0; i < count; ++i)
        {
            MyGUI::Widget* w = getChildAt(i);
            bool hstretch = w->getUserString("HStretch") == "true";
            bool hidden = w->getUserString("Hidden") == "true";
            if (hidden)
                continue;
            hStretchedCount += hstretch;
            AutoSizedWidget* aw = dynamic_cast<AutoSizedWidget*>(w);
            if (aw)
            {
                sizes[i] = std::make_pair(aw->getRequestedSize(), hstretch);
                totalWidth += aw->getRequestedSize().width;
                totalHeight = std::max(totalHeight, aw->getRequestedSize().height);
            }
            else
            {
                sizes[i] = std::make_pair(w->getSize(), hstretch);
                totalWidth += w->getSize().width;
                if (!(w->getUserString("VStretch") == "true"))
                    totalHeight = std::max(totalHeight, w->getSize().height);
            }

            if (i != count - 1)
                totalWidth += mSpacing;
        }

        if (mAutoResize
            && (totalWidth + mPadding * 2 != getClientCoord().width
                || totalHeight + mPadding * 2 != getClientCoord().height))
        {
            int xmargin = getSize().width - getClientCoord().width;
            int ymargin = getSize().height - getClientCoord().height;
            setSize(MyGUI::IntSize(totalWidth + mPadding * 2 + xmargin, totalHeight + mPadding * 2 + ymargin));
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

            bool vstretch = w->getUserString("VStretch") == "true";
            int maxHeight = getClientCoord().height - mPadding * 2;
            int height = vstretch ? maxHeight : sizes[i].first.height;

            MyGUI::IntCoord widgetCoord;
            widgetCoord.left = curX;
            widgetCoord.top = mPadding + (getClientCoord().height - mPadding * 2 - height) / 2;

            int width = 0;
            if (sizes[i].second)
            {
                if (hStretchedCount == 0)
                    throw std::logic_error("unexpected");
                width = sizes[i].first.width + (getClientCoord().width - mPadding * 2 - totalWidth) / hStretchedCount;
            }
            else
                width = sizes[i].first.width;

            widgetCoord.width = width;
            widgetCoord.height = height;
            w->setCoord(widgetCoord);
            curX += width;

            if (i != count - 1)
                curX += mSpacing;
        }
    }

    void HBox::setPropertyOverride(std::string_view key, std::string_view value)
    {
        if (!Box::_setPropertyImpl(key, value))
            MyGUI::Widget::setPropertyOverride(key, value);
    }

    void HBox::setSize(const MyGUI::IntSize& value)
    {
        MyGUI::Widget::setSize(value);
        align();
    }

    void HBox::setCoord(const MyGUI::IntCoord& value)
    {
        MyGUI::Widget::setCoord(value);
        align();
    }

    void HBox::initialiseOverride()
    {
        Base::initialiseOverride();
        MyGUI::Widget* client = nullptr;
        assignWidget(client, "Client");
        setWidgetClient(client);
    }

    void HBox::onWidgetCreated(MyGUI::Widget* /*widget*/)
    {
        align();
    }

    MyGUI::IntSize HBox::getRequestedSize()
    {
        MyGUI::IntSize size(0, 0);
        for (unsigned int i = 0; i < getChildCount(); ++i)
        {
            bool hidden = getChildAt(i)->getUserString("Hidden") == "true";
            if (hidden)
                continue;

            AutoSizedWidget* w = dynamic_cast<AutoSizedWidget*>(getChildAt(i));
            if (w)
            {
                MyGUI::IntSize requested = w->getRequestedSize();
                size.height = std::max(size.height, requested.height);
                size.width = size.width + requested.width;
                if (i != getChildCount() - 1)
                    size.width += mSpacing;
            }
            else
            {
                MyGUI::IntSize requested = getChildAt(i)->getSize();
                size.height = std::max(size.height, requested.height);

                if (getChildAt(i)->getUserString("HStretch") != "true")
                    size.width = size.width + requested.width;

                if (i != getChildCount() - 1)
                    size.width += mSpacing;
            }
            size.height += mPadding * 2;
            size.width += mPadding * 2;
        }
        return size;
    }

    void VBox::align()
    {
        unsigned int count = getChildCount();
        size_t vStretchedCount = 0;
        int totalHeight = 0;
        int totalWidth = 0;
        std::vector<std::pair<MyGUI::IntSize, bool>> sizes;
        sizes.resize(count);
        for (unsigned int i = 0; i < count; ++i)
        {
            MyGUI::Widget* w = getChildAt(i);

            bool hidden = w->getUserString("Hidden") == "true";
            if (hidden)
                continue;

            bool vstretch = w->getUserString("VStretch") == "true";
            vStretchedCount += vstretch;
            AutoSizedWidget* aw = dynamic_cast<AutoSizedWidget*>(w);
            if (aw)
            {
                sizes[i] = std::make_pair(aw->getRequestedSize(), vstretch);
                totalHeight += aw->getRequestedSize().height;
                totalWidth = std::max(totalWidth, aw->getRequestedSize().width);
            }
            else
            {
                sizes[i] = std::make_pair(w->getSize(), vstretch);
                totalHeight += w->getSize().height;

                if (!(w->getUserString("HStretch") == "true"))
                    totalWidth = std::max(totalWidth, w->getSize().width);
            }

            if (i != count - 1)
                totalHeight += mSpacing;
        }

        if (mAutoResize
            && (totalWidth + mPadding * 2 != getClientCoord().width
                || totalHeight + mPadding * 2 != getClientCoord().height))
        {
            int xmargin = getSize().width - getClientCoord().width;
            int ymargin = getSize().height - getClientCoord().height;
            setSize(MyGUI::IntSize(totalWidth + mPadding * 2 + xmargin, totalHeight + mPadding * 2 + ymargin));
            return;
        }

        int curY = 0;
        for (unsigned int i = 0; i < count; ++i)
        {
            if (i == 0)
                curY += mPadding;

            MyGUI::Widget* w = getChildAt(i);

            bool hidden = w->getUserString("Hidden") == "true";
            if (hidden)
                continue;

            bool hstretch = w->getUserString("HStretch") == "true";
            int maxWidth = getClientCoord().width - mPadding * 2;
            int width = hstretch ? maxWidth : sizes[i].first.width;

            MyGUI::IntCoord widgetCoord;
            widgetCoord.top = curY;
            widgetCoord.left = mPadding + (getClientCoord().width - mPadding * 2 - width) / 2;

            int height = 0;
            if (sizes[i].second)
            {
                if (vStretchedCount == 0)
                    throw std::logic_error("unexpected");
                height
                    = sizes[i].first.height + (getClientCoord().height - mPadding * 2 - totalHeight) / vStretchedCount;
            }
            else
                height = sizes[i].first.height;

            widgetCoord.height = height;
            widgetCoord.width = width;
            w->setCoord(widgetCoord);
            curY += height;

            if (i != count - 1)
                curY += mSpacing;
        }
    }

    void VBox::setPropertyOverride(std::string_view key, std::string_view value)
    {
        if (!Box::_setPropertyImpl(key, value))
            MyGUI::Widget::setPropertyOverride(key, value);
    }

    void VBox::setSize(const MyGUI::IntSize& value)
    {
        MyGUI::Widget::setSize(value);
        align();
    }

    void VBox::setCoord(const MyGUI::IntCoord& value)
    {
        MyGUI::Widget::setCoord(value);
        align();
    }

    void VBox::initialiseOverride()
    {
        Base::initialiseOverride();
        MyGUI::Widget* client = nullptr;
        assignWidget(client, "Client");
        setWidgetClient(client);
    }

    MyGUI::IntSize VBox::getRequestedSize()
    {
        MyGUI::IntSize size(0, 0);
        for (unsigned int i = 0; i < getChildCount(); ++i)
        {
            bool hidden = getChildAt(i)->getUserString("Hidden") == "true";
            if (hidden)
                continue;

            AutoSizedWidget* w = dynamic_cast<AutoSizedWidget*>(getChildAt(i));
            if (w)
            {
                MyGUI::IntSize requested = w->getRequestedSize();
                size.width = std::max(size.width, requested.width);
                size.height = size.height + requested.height;
                if (i != getChildCount() - 1)
                    size.height += mSpacing;
            }
            else
            {
                MyGUI::IntSize requested = getChildAt(i)->getSize();
                size.width = std::max(size.width, requested.width);

                if (getChildAt(i)->getUserString("VStretch") != "true")
                    size.height = size.height + requested.height;

                if (i != getChildCount() - 1)
                    size.height += mSpacing;
            }
            size.height += mPadding * 2;
            size.width += mPadding * 2;
        }
        return size;
    }

    void VBox::onWidgetCreated(MyGUI::Widget* /*widget*/)
    {
        align();
    }

    Spacer::Spacer()
    {
        setUserString("HStretch", "true");
        setUserString("VStretch", "true");
    }

}
