
#include "pushbutton.hpp"

#include <QMouseEvent>
#include <QKeyEvent>

void CSVWidget::PushButton::setExtendedToolTip (const std::string& text)
{
    std::string tooltip = text;

    if (tooltip.empty())
        tooltip = "(Tool tip not implemented yet)";

    switch (mType)
    {
        case Type_TopMode:

            tooltip +=
                "<p>(left click to change mode)";

            break;

        case Type_Mode:

            tooltip +=
                "<p>(left click to activate,"
                "<br>shift-left click to activate and keep panel open)";

            break;
    }

    setToolTip (QString::fromUtf8 (tooltip.c_str()));
}

void CSVWidget::PushButton::keyPressEvent (QKeyEvent *event)
{
    if (event->key()!=Qt::Key_Shift)
        mKeepOpen = false;

    QPushButton::keyPressEvent (event);
}

void CSVWidget::PushButton::keyReleaseEvent (QKeyEvent *event)
{
    if (event->key()==Qt::Key_Return || event->key()==Qt::Key_Enter)
    {
        mKeepOpen = event->modifiers() & Qt::ShiftModifier;
        emit clicked();
    }

    QPushButton::keyReleaseEvent (event);
}

void CSVWidget::PushButton::mouseReleaseEvent (QMouseEvent *event)
{
    mKeepOpen = event->button()==Qt::LeftButton && (event->modifiers() & Qt::ShiftModifier);
    QPushButton::mouseReleaseEvent (event);
}

CSVWidget::PushButton::PushButton (const QIcon& icon, Type type, const std::string& tooltip,
    QWidget *parent)
: QPushButton (icon, "", parent), mKeepOpen (false), mType (type)
{
    setCheckable (type==Type_Mode);
    setExtendedToolTip (tooltip);
}

CSVWidget::PushButton::PushButton (Type type, const std::string& tooltip, QWidget *parent)
: QPushButton (parent), mKeepOpen (false), mType (type)
{
    setCheckable (type==Type_Mode);
    setExtendedToolTip (tooltip);
}

bool CSVWidget::PushButton::hasKeepOpen() const
{
    return mKeepOpen;
}